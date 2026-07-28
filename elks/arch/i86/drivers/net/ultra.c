/*
 * SMC Ultra/UltraChip 83C790 Ethernet driver for ELKS.
 *
 * The SMC Ultra family uses an 8390-compatible NIC core, but the board ASIC
 * is not WD/SMC 80x3 compatible.  The station address PROM is at base+8,
 * the Ultra ID is at base+7, the 8390 registers start at base+16, and the
 * shared-memory aperture is enabled through the Ultra command register.
 * Register-level probe and startup logic follows Linux 2.0 smc-ultra.c.
 */

#include <arch/io.h>
#include <arch/irq.h>
#include <arch/ports.h>
#include <arch/segment.h>
#include <linuxmt/config.h>
#include <linuxmt/memory.h>
#include <linuxmt/errno.h>
#include <linuxmt/major.h>
#include <linuxmt/ioctl.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/fs.h>
#include <linuxmt/sched.h>
#include <linuxmt/limits.h>
#include <linuxmt/mm.h>
#include <linuxmt/debug.h>
#include <linuxmt/netstat.h>
#include <linuxmt/timer.h>
#include "eth-msgs.h"
#include "8390.h"

/*
 * Poll interval for the receive-watchdog timer.  The driver is normally
 * interrupt driven, but ktcp blocks in select() with no timeout when it is
 * idle, so a card whose interrupt never reaches the host goes permanently
 * deaf to inbound traffic while still working for anything the guest itself
 * initiates.  Machines exist where that happens -- the Amstrad PC1640 is
 * one -- so poll the ring as a safety net and wake the readers if a frame
 * arrived without an interrupt.
 */
#define ULTRA_POLL_TICKS        (HZ / 5)

/*
 * Cap on one pass through the interrupt handler.  Each iteration services
 * one condition and writes its status bit back, so a healthy card needs
 * only a handful; anything beyond this means the chip is not clearing.
 */
#define ULTRA_INT_MAXLOOP       20

/* runtime configuration set in /bootopts or defaults in ports.h */
#define net_irq		(netif_parms[ETH_ULTRA].irq)
#define net_port	(netif_parms[ETH_ULTRA].port)
#define net_ram		(netif_parms[ETH_ULTRA].ram)
#define net_flags	(netif_parms[ETH_ULTRA].flags)

#define ULTRA_STAT_RX	0x0001	/* packet received */
#define ULTRA_STAT_TX	0x0002	/* packet sent */
#define ULTRA_STAT_OF	0x0010	/* RX ring overflow */

#define ULTRA_START_PG	0x00U	/* First page of TX buffer */
#define ULTRA_STOP_PG4	0x10U	/* Only used for arithmetic */
#define ULTRA_STOP_PG8	0x20U	/* Last page + 1 of RX ring if 8 bit i/f */
#define ULTRA_STOP_PG16	0x40U	/* Last page + 1 of RX ring if 16bit i/f */
#define ULTRA_STOP_PG32	0x80U	/* Last page + 1 of RX ring if 32K buf space */

#define TX_2X_PAGES	12		/* useful if 16+k buffer */
#define TX_1X_PAGES	6
#define TX_PAGES	TX_1X_PAGES	/* use one packet buffer for xmit */

#define ULTRA_FIRST_TX_PG	ULTRA_START_PG
#define ULTRA_FIRST_RX_PG	(ULTRA_FIRST_TX_PG + TX_PAGES)

#define ULTRA_RESET	0x80U	/* Board reset */
#define ULTRA_MEMENB	0x40U	/* Enable the shared memory */
#define ULTRA_IO_EXTENT	32
#define ULTRA_8390_OFFSET	16
#define ULTRA_8390_PORT	(net_port + ULTRA_8390_OFFSET)
#define ULTRA_IDREG	7
#define ULTRA_MEM_CTRL	6
#define ULTRA_CFG	4
#define ULTRA_CFG_ALT	0x80U
#define EN0_ERWCNT	0x08	/* Early receive warning count (Ultra ASIC) */

#define DEBUG 0

typedef struct {
	unsigned char status;	/* status */
	unsigned char next;	/* pointer to next packet */
	unsigned short count;	/* header + packet length in bytes */
} __attribute__((packed)) e8390_pkt_hdr;

static struct wait_queue rxwait;
static struct wait_queue txwait;

static byte_t usecount;
static byte_t use_shared_mem;
static byte_t model_name[] = "smc-ultra";
static byte_t dev_name[] = "ul0";
static byte_t stop_page; 	/* actual last pg of ring (+1) */
static unsigned char found;
static unsigned int verbose;

static unsigned char current_rx_page;
static struct netif_stat netif_stat;
static unsigned char model_is_ez;
static unsigned char ultra_polled_rx;   /* set once a frame arrives w/o irq */
static unsigned char ultra_irq_forced;  /* /bootopts overrode the card's irq */
static void ultra_poll_timer(void);
static struct timer_list ultra_rx_poll = { NULL, 0, NULL, ultra_poll_timer };

static void NICPROC ultra_stat_add(__u16 *stat, unsigned int count)
{
	if (count >= 0xffffU - *stat)
		*stat = 0xffffU;
	else
		*stat += count;
}

static void NICPROC ultra_stat_inc(__u16 *stat)
{
	ultra_stat_add(stat, 1);
}

static void NICPROC ultra_drain_err_counters(void)
{
	unsigned int errors;

	errors = inb(ULTRA_8390_PORT + EN0_COUNTER0);
	errors += inb(ULTRA_8390_PORT + EN0_COUNTER1);
	errors += inb(ULTRA_8390_PORT + EN0_COUNTER2);
	if (errors)
		ultra_stat_add(&netif_stat.rx_errors, errors);
}

static word_t NICPROC ultra_rx_stat(void);
static word_t NICPROC ultra_tx_stat(void);
static void ultra_int(int irq, struct pt_regs * regs);
static void NICPROC fmemcpy(void *, seg_t, void *, seg_t, size_t, int);
static void NICPROC ultra_stop(void);

static void NICPROC ultra_mem_on(void)
{
	outb(ULTRA_MEMENB, net_port);
}

static void NICPROC ultra_mem_off(void)
{
	outb(0, net_port);
}

extern struct eth eths[];

/*
 * Get MAC
 */

static void NICPROC ultra_get_hw_addr(word_t *data)
{
	unsigned u;

	for (u = 0; u < 6; u++)
		data[u] = inb(net_port + 8 + u);
}

/*
 * Check if card exists, get MAC addr from PROM,
 * determine bus width if possible.
 */

static int INITPROC ultra_probe(void) {
	static unsigned char ultra_irqmap[] = { 0, 9, 3, 5, 7, 10, 11, 15 };
	static unsigned long ultra_addrtbl[] = {
		0x0c0000UL, 0x0e0000UL, 0xfc0000UL, 0xfe0000UL
	};
	static unsigned char ultra_pages[] = { 0x20, 0x40, 0x80, 0xff };
	int i, tmp = 0;
	unsigned long mem;
	unsigned char id, reg4, reg8, reg11, reg13;

#if DEBUG
	for (i = 0; i < 16; i++)
		printk("%x;", inb(net_port+i));
	printk("\n");
#endif
	id = inb(net_port + ULTRA_IDREG);
	if ((id & 0xf0) != 0x20 && (id & 0xf0) != 0x40)
		return -ENODEV;
	model_is_ez = (id & 0xf0) == 0x40;

	/* Select the station address register set before reading the PROM. */
	reg4 = inb(net_port + ULTRA_CFG) & ~ULTRA_CFG_ALT;
	outb(reg4, net_port + ULTRA_CFG);
	for (i = 0; i < 8; i++)
		tmp += inb(net_port + 8 + i);
	if (inb(net_port + 8) == 0xff
		|| inb(net_port + 9) == 0xff	/* Extra check to avoid soundcard. */
		|| (tmp & 0xff) != 0xFF)	/* checksum test */
		return -ENODEV;	

	/* config flag processing */
	verbose = (net_flags&ETHF_VERBOSE);	/* set verbose messages */

	outb(reg4 | ULTRA_CFG_ALT, net_port + ULTRA_CFG);
	outb(0x80U | inb(net_port + 0x0c), net_port + 0x0c);
	reg8 = inb(net_port + 8);
	reg11 = inb(net_port + 11);
	reg13 = inb(net_port + 13);
	outb(reg4, net_port + ULTRA_CFG);

	if (!net_ram) {
		mem = ultra_addrtbl[(reg11 >> 6) & 0x03]
			+ ((unsigned long)(reg11 & 0x0f) << 13);
		net_ram = (unsigned int)(mem >> 4);
	}
	if (net_irq <= 1)
		net_irq = ultra_irqmap[((reg13 & 0x40) >> 4) | ((reg13 & 0x0c) >> 2)];
	else if (net_irq != ultra_irqmap[((reg13 & 0x40) >> 4) | ((reg13 & 0x0c) >> 2)])
		ultra_irq_forced = 1;	/* only then rewrite the board register */
	if (!net_irq) {
		printk(" no irq");
		return -EAGAIN;
	}
	if (reg8) {
		printk(" pio unsupported");
		return -ENODEV;
	}

	use_shared_mem = 1;
	stop_page = ultra_pages[(reg11 >> 4) & 0x03];
	if (net_flags&0x07)		/* Force buffer size */
		stop_page = ULTRA_STOP_PG4 << (net_flags&0x03);
	if (stop_page <= ULTRA_STOP_PG4)
		netif_stat.oflow_keep = 0;
	else if (stop_page <= ULTRA_STOP_PG8)
		netif_stat.oflow_keep = 1;
	else
		netif_stat.oflow_keep = 3;
	netif_stat.if_status = NETIF_AUTO_8BIT;
	if (stop_page <= ULTRA_STOP_PG4)
		netif_stat.if_status |= 1;
	else if (stop_page <= ULTRA_STOP_PG8)
		netif_stat.if_status |= 2;
	else if (stop_page <= ULTRA_STOP_PG16)
		netif_stat.if_status |= 3;
	else if (stop_page <= ULTRA_STOP_PG32)
		netif_stat.if_status |= 4;
	else
		netif_stat.if_status |= 5;
#if DEBUG
	printk("net_flags %04x stop_page %02x verbose %d\n", net_flags, stop_page, verbose);
#endif
	ultra_stop();	/* make sure interrupts are off */

	return 0;
}

/*
 * Reset
 */

static void NICPROC ultra_reset(void)
{
	outb(ULTRA_RESET, net_port);

	ultra_mem_off();
	outb(0x80U, net_port + 5);
	outb(0x01U, net_port + ULTRA_MEM_CTRL);
}

/*
 * Init	- basic 8390 initialization. If the strategy parameter is 0, do complete
 *	  initialization, otherwise just recover from a receive overflow and 
 *	  keep 'strategy' # of packets in the buffer.
 */

static void NICPROC ultra_init_8390(int strategy)
{
	unsigned u;
	const e8390_pkt_hdr __far *rxhdr;
	word_t hdr_start;
	byte_t *mac_addr = (byte_t *)&netif_stat.mac_addr;
	flag_t flags;

	outb(E8390_NODMA | E8390_PAGE0 | E8390_STOP,
		ULTRA_8390_PORT + E8390_CMD);
	/*
	 * The 8390 finishes any DMA burst in progress before it really
	 * stops.  Programming its registers immediately, as this did, can
	 * leave the chip wedged so that it answers with a stuck status
	 * forever after.  Linux waits here; give it settling time too.
	 */
	for (u = 0; u < 2000; u++)
		(void)inb(ULTRA_8390_PORT + EN0_ISR);
	outb(0x49, ULTRA_8390_PORT + EN0_DCFG);

	/* Clear the remote byte count registers. */
	outb(0x00, ULTRA_8390_PORT + EN0_RCNTLO);
	outb(0x00, ULTRA_8390_PORT + EN0_RCNTHI);

	/* Set to monitor and loopback mode. */
	outb(E8390_RXOFF, ULTRA_8390_PORT + EN0_RXCR);
	outb(E8390_TXOFF, ULTRA_8390_PORT + EN0_TXCR);

	/* Clear the pending interrupts and mask. */
	outb(0xff, ULTRA_8390_PORT + EN0_ISR);
	outb(0x00, ULTRA_8390_PORT + EN0_IMR);

	if (strategy == 0) {	/* full initialization */
		/* Set the transmit page and receive ring. */
		outb(ULTRA_FIRST_TX_PG, ULTRA_8390_PORT + EN0_TPSR);
		outb(ULTRA_FIRST_RX_PG, ULTRA_8390_PORT + EN0_STARTPG);
		outb(stop_page, ULTRA_8390_PORT + EN0_STOPPG);
		current_rx_page = ULTRA_FIRST_RX_PG;
		outb(stop_page - 1, ULTRA_8390_PORT + EN0_BOUNDARY);


		/* Copy the station address into the DS8390 registers. */
		save_flags(flags);
		clr_irq();
		outb(E8390_NODMA | E8390_PAGE1 | E8390_STOP,
			ULTRA_8390_PORT + E8390_CMD);
		for (u = 0U; u < 6U; u++)
			outb(mac_addr[u], ULTRA_8390_PORT + EN1_PHYS + u);
		outb(ULTRA_FIRST_RX_PG, ULTRA_8390_PORT + EN1_CURPAG);
		outb(E8390_NODMA | E8390_PAGE0 | E8390_STOP,
			ULTRA_8390_PORT + E8390_CMD);
		restore_flags(flags);

	} else {	/* 'strategy' is the # of packets to keep. */
			/* This is for overflow recovery */
		unsigned char next_page = current_rx_page;

		ultra_mem_on();
		for (u = strategy; u; u--) {
			hdr_start = (next_page - ULTRA_START_PG) << 8U;
			rxhdr = _MK_FP(net_ram, hdr_start);
			next_page = rxhdr->next;
			if (next_page < ULTRA_FIRST_RX_PG || next_page >= stop_page) {
				next_page = current_rx_page;
				break;
			}
		}
		ultra_mem_off();

		outb(E8390_NODMA | E8390_PAGE1 | E8390_STOP,
			ULTRA_8390_PORT + E8390_CMD);
		outb(next_page, ULTRA_8390_PORT + EN1_CURPAG);
		outb(E8390_NODMA | E8390_PAGE0 | E8390_STOP,
			ULTRA_8390_PORT + E8390_CMD);
	}

}

/*
 * Start
 */

static void NICPROC ultra_start(void)
{
	static unsigned char ultra_irqreg[] = {
		0x00, 0x00, 0x04, 0x08, 0x00, 0x0c, 0x00, 0x40,
		0x00, 0x04, 0x44, 0x48, 0x00, 0x00, 0x00, 0x4c
	};
	unsigned char reg4;

	ultra_mem_off();
	outb(0x80U, net_port + 5);
	/*
	 * Only force the board's interrupt select when /bootopts asked for an
	 * IRQ the card is not already configured for.  Linux smc-ultra reads
	 * this register and never writes it, and on at least one 8216 the
	 * write leaves the card raising its 8390 interrupt status while
	 * driving nothing onto the bus -- the adapter looks alive but no
	 * handler ever runs.  Trust the card's own configuration by default.
	 */
	if (!ultra_irq_forced) {
		outb(0x01, net_port + ULTRA_MEM_CTRL);
		goto started;
	}
	if (net_irq >= 0 && net_irq < sizeof(ultra_irqreg)
	    && ultra_irqreg[net_irq]) {
		reg4 = inb(net_port + ULTRA_CFG) & ~ULTRA_CFG_ALT;
		outb(reg4 | ULTRA_CFG_ALT, net_port + ULTRA_CFG);
		outb((inb(net_port + 13) & ~0x4cU) | ultra_irqreg[net_irq],
			net_port + 13);
		outb(reg4, net_port + ULTRA_CFG);
	}
	outb(0x01, net_port + ULTRA_MEM_CTRL);
started:
	outb(E8390_NODMA | E8390_PAGE0, ULTRA_8390_PORT + E8390_CMD);
	outb(0xffU, ULTRA_8390_PORT + EN0_ERWCNT);

	outb(E8390_TXCONFIG, ULTRA_8390_PORT + EN0_TXCR); /* xmit on */
	outb(E8390_RXCONFIG, ULTRA_8390_PORT + EN0_RXCR); /* rx on */

	outb(E8390_NODMA | E8390_PAGE0 | E8390_START, ULTRA_8390_PORT + E8390_CMD);
	outb(ENISR_ALL, ULTRA_8390_PORT + EN0_IMR);	/* enable interrupts */
}

/*
 * Stop & terminate
 */

static void NICPROC ultra_stop(void)
{
	outb(0, ULTRA_8390_PORT + EN0_IMR);	/* mask all interrupts */
	outb(0, net_port + ULTRA_MEM_CTRL);
	ultra_mem_off();
}

/*
 * Receive watchdog.  Re-arms itself while the device is open; wakes any
 * blocked reader whenever the card's current page has moved past ours,
 * which is exactly the condition ultra_rx_stat() reports.
 */
static void ultra_poll_timer(void)
{
	word_t stat;

	if (!usecount)
		return;			/* closed: let the timer lapse */

	/*
	 * Service anything the card is asking for that no interrupt came in
	 * to handle.  Running the real handler rather than just waking the
	 * readers matters: an unserviced ring fills, the 8390 raises OFLOW
	 * and then stops storing frames altogether, and the recovery for
	 * that lives inside the handler.  A card whose interrupt is dead
	 * would otherwise go permanently deaf the first time it overflowed.
	 */
	stat = inb(ULTRA_8390_PORT + EN0_ISR);
	if (stat & ENISR_ALL) {
		if (!ultra_polled_rx) {
			ultra_polled_rx = 1;
			printk("%s: servicing rx by poll, irq %d not arriving\n",
				dev_name, net_irq);
		}
		ultra_int(net_irq, NULL);
	} else if (ultra_rx_stat() == ULTRA_STAT_RX)
		wake_up(&rxwait);	/* frame present, status already clear */

	ultra_rx_poll.tl_expires = jiffies() + ULTRA_POLL_TICKS;
	add_timer(&ultra_rx_poll);
}

/*
 * Clear overflow
 *
 *	For ELKS, when an overflow occurs, the kernel will probably just have
 *	received a wakeup() from a RxComplete interrupt. 
 *	If the overflow handler purges the receive buffer
 *	completely, the next read will fail - there is nothing to read. No big
 *	deal, but noisy (error messages) and inefficient since the buffer is at
 *	least 8k. So in most cases we let 1 or more packets survive the overflow 
 *	recovery (as specified by the parameter to ultra_init_8390() ).
 */

static void NICPROC ultra_clr_oflow(int keep)
{
	ultra_init_8390(keep);
	ultra_start();
}

/*
 * Get packet
 */

static size_t NICPROC ultra_pack_get(char *data, size_t len)
{
	const e8390_pkt_hdr __far *rxhdr;
	word_t hdr_start;
	unsigned char this_frame, update = 1;
	size_t res = -EIO;

	outb(0x00, ULTRA_8390_PORT + EN0_IMR);	/* block interrupts */
	if (use_shared_mem)
		ultra_mem_on();
	do {
		/* Remove one frame from the ring. */
		/* Boundary is always a page behind. */
		this_frame = inb(ULTRA_8390_PORT + EN0_BOUNDARY) + 1U;
		if (this_frame >= stop_page)
			this_frame = ULTRA_FIRST_RX_PG;
		if (this_frame != current_rx_page)	/* Very useful for debugging ! */
			printk("eth: mismatched read page pointers %2x vs %2x.\n",
				this_frame, current_rx_page);
		hdr_start = (this_frame - ULTRA_START_PG) << 8;
		rxhdr = _MK_FP(net_ram, hdr_start);

		if ((rxhdr->count < 64) ||
		    (rxhdr->count > (MAX_PACKET_ETH + sizeof(e8390_pkt_hdr)))) {

			/* This should not happen! The NIC is programmed to drop
			 * erroneous packets. If we get here, it's most likely
			 * a driver bug or a hardware problem. */
			/* If this happens, we need to purge the NIC buffer entirely 
			 * since if the size is bogus, the next packet pointer is
			 * unreliable at best. */
			ultra_stat_inc(&netif_stat.rq_errors);
			if (verbose) printk(EMSG_DMGPKT, dev_name, (unsigned int *)rxhdr, rxhdr->next);
			
			ultra_mem_off();
			ultra_clr_oflow(0);	/* Complete reset */
			update = 0;		/* exit flag */
			break;
		}
		current_rx_page = rxhdr->next;
		if ((rxhdr->status & 0x0fU) != ENRSR_RXOK) {
			/* This shouldn't happen either, see comment above */
			ultra_stat_inc(&netif_stat.rx_errors);
			if (verbose) printk(EMSG_BGSPKT, dev_name,
				rxhdr->status, rxhdr->next, rxhdr->count);
			break;
		} 
		res = rxhdr->count - sizeof(e8390_pkt_hdr);
		if (res > len) res = len;
		if (current_rx_page > this_frame || current_rx_page == ULTRA_FIRST_RX_PG) {
			/* no wrap around */
			fmemcpy(data, current->t_regs.ds,
				(char *)hdr_start + sizeof(e8390_pkt_hdr), net_ram, res, 0);
		} else {	/* handle wrap-around */
			size_t len1 = ((stop_page - this_frame) << 8) - sizeof(e8390_pkt_hdr);
			fmemcpy(data, current->t_regs.ds,
				(char *)hdr_start + sizeof(e8390_pkt_hdr), net_ram, len1, 0);
			fmemcpy(data+len1, current->t_regs.ds,
				(char *)(ULTRA_FIRST_RX_PG << 8), net_ram, res-len1, 0);
 		}
	} while (0);

	if (update) {	/* don't update if we ran overflow recovery */
		this_frame = (current_rx_page == ULTRA_FIRST_RX_PG) ? stop_page - 1 : current_rx_page - 1;
		outb(this_frame, ULTRA_8390_PORT + EN0_BOUNDARY);
	}
	ultra_mem_off();
	outb(ENISR_ALL, ULTRA_8390_PORT + EN0_IMR);

	return res;
}

static size_t ultra_read(struct inode * inode, struct file * filp,
	char * data, size_t len)
{
	size_t res = 0;

	do {
		prepare_to_wait_interruptible(&rxwait);
		if (ultra_rx_stat() != ULTRA_STAT_RX) {
			if (filp->f_flags & O_NONBLOCK) {
				res = -EAGAIN;
				break;
			}
			do_wait();
			if (current->signal) {
				res = -EINTR;
				break;
			}
		}
		res = ultra_pack_get(data, len);	/* returns packet data size read */
	} while (0);

	finish_wait(&rxwait);
	return res;
}

/*
 * Pass packet to driver for send
 */

static size_t NICPROC ultra_pack_put(char *data, size_t len)
{
	do {
		if (len > MAX_PACKET_ETH)
			len = MAX_PACKET_ETH;
		if (len < 64U) len = 64U;  /* issue #133 */

		if (use_shared_mem)
			ultra_mem_on();
		fmemcpy((byte_t *)((ULTRA_FIRST_TX_PG - ULTRA_START_PG) << 8U),
			net_ram, data, current->t_regs.ds, len, 0);
		ultra_mem_off();
		outb(E8390_NODMA | E8390_PAGE0, ULTRA_8390_PORT + E8390_CMD);

#if REMOVE
		/* FIXME: superfluous. Cannot get here unless we have a trans complete intr */
		/* which means the NIC is ready for more */
		if (inb(ULTRA_8390_PORT + E8390_CMD) & E8390_TRANS) {
			printk("eth: attempted send with the tr busy.\n");
			len = -EIO;
			break;
		}
#endif
		outb(len & 0xffU, ULTRA_8390_PORT + EN0_TCNTLO);
		outb(len >> 8U, ULTRA_8390_PORT + EN0_TCNTHI);
		outb(ULTRA_FIRST_TX_PG, ULTRA_8390_PORT + EN0_TPSR);
		outb(E8390_NODMA | E8390_TRANS, ULTRA_8390_PORT + E8390_CMD);
	} while (0);
	return len;
}

static size_t ultra_write(struct inode * inode, struct file * file,
	char * data, size_t len)
{
	int res;

	do {
		prepare_to_wait_interruptible(&txwait);
		if (ultra_tx_stat() != ULTRA_STAT_TX) {
			if (file->f_flags & O_NONBLOCK) {
				res = -EAGAIN;
				break;
			}
			do_wait();
			if(current->signal) {
				res = -EINTR;
				break;
			}
		}
		res = ultra_pack_put(data, len);
	} while (0);
	finish_wait(&txwait);
	return res;
}

/*
 * Test for readiness
 */

static word_t NICPROC ultra_rx_stat(void)
{
	unsigned char rxing_page;
	flag_t flags;

	save_flags(flags);
	clr_irq();	/* only our own interrupts need blocking here */
	/* Get the rx page (incoming packet pointer). */
	outb(E8390_NODMA | E8390_PAGE1, ULTRA_8390_PORT + E8390_CMD);
	rxing_page = inb(ULTRA_8390_PORT + EN1_CURPAG);
	outb(E8390_NODMA | E8390_PAGE0, ULTRA_8390_PORT + E8390_CMD);
	restore_flags(flags);

	return (current_rx_page == rxing_page) ? 0 : ULTRA_STAT_RX;
}

static word_t NICPROC ultra_tx_stat(void)
{
	return (inb(ULTRA_8390_PORT + E8390_CMD) & E8390_TRANS) ? 0 :
		ULTRA_STAT_TX;
}

static int ultra_select(struct inode * inode, struct file * filp, int sel_type)
{
	int res = 0;

	switch (sel_type) {
		case SEL_OUT:
			if (ultra_tx_stat() != ULTRA_STAT_TX) {
				select_wait(&txwait);
				break;
			}
			res = 1;
			break;
		case SEL_IN:
			if (ultra_rx_stat() != ULTRA_STAT_RX) {
				select_wait(&rxwait);
				break;
			}
			res = 1;
			break;
		default:
			res = -EINVAL;
	}
	return res;
}

static int ultra_net_start(void)
{
	int err;

	if (usecount)
		return 0;

	err = request_irq(net_irq, ultra_int, INT_GENERIC);
	if (err) {
		printk(EMSG_IRQERR, dev_name, net_irq, err);
		return err;
	}
	ultra_reset();
	ultra_init_8390(0);
	ultra_start();
	usecount = 1;
	ultra_rx_poll.tl_expires = jiffies() + ULTRA_POLL_TICKS;
	add_timer(&ultra_rx_poll);
	return 0;
}

/*
 * I/O control
 */

static int ultra_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned int arg)
{
	int err = 0;

	switch (cmd) {
	case IOCTL_ETH_ADDR_GET:
		err = verified_memcpy_tofs((char *)arg, &netif_stat.mac_addr, 6U);
		break;

#if 0 /* unused*/
	case IOCTL_ETH_ADDR_SET:
		err = -ENOSYS;
		break;

	case IOCTL_ETH_HWADDR_GET:
		/* Get the hardware address of the NIC,	which may be different
		 * from the currently programmed address. Be careful with this,
		 * it may interrupt ongoing send/receives.
		 * arg must be a 6 word array.
		 */
		ultra_get_hw_addr((word_t *)arg);
		break;
#endif

	case IOCTL_ETH_GETSTAT:
		/* Get error counts etc. */
		/* FIXME: add mcounts recorded by the NIC */
		err = verified_memcpy_tofs((char *)arg, &netif_stat, sizeof(netif_stat));
		break;

	default:
		err = -EINVAL;
	}
	return err;
}

/*
 * Device open
 */

static int ultra_open(struct inode *inode, struct file *file)
{
	int err = 0;

	do {
		if (!found) {
			err = -ENODEV;
			break;
		}
		if (usecount) {
			usecount++;
			break;
		}
		err = ultra_net_start();
	} while (0);
#if DEBUG
	printk("ul0: open status %d\n", err);
#endif
	return err;
}

/*
 * Release (close) device
 */

static void ultra_release(struct inode *inode, struct file *file)
{
#if DEBUG
	printk("ul0: release: usecnt %d\n", usecount);
#endif
	if (--usecount == 0) {
		del_timer(&ultra_rx_poll);
		ultra_stop();
		free_irq(net_irq);
	}
}

/*
 * Ethernet operations
 */

struct file_operations ultra_fops =
{
	NULL,         /* lseek */
	ultra_read,
	ultra_write,
	NULL,         /* readdir */
	ultra_select,
	ultra_ioctl,
	ultra_open,
	ultra_release
};

/*
 * Interrupt handler
 *
 * The logic behind clearing each interrupt bit as we handle them is
 * that since we're (presumably) on a slow machine, a new interrupt may
 * occur while processing. It will not trigger a physical interrupt since they're
 * disabled, but the corresponding ISR bit will be set again and will be processed in the 
 * next round of the while-loop, thus avoiding an occasional lost interrupt.
 */

static void ultra_int(int irq, struct pt_regs * regs)
{
	word_t stat;
	unsigned int loops;

	outb(0, ULTRA_8390_PORT + EN0_IMR);/* Block interrupts,
					 * should not be required since the IRQ line
					 * is held high until all unmasked bits have
					 * been cleared. This is experimental.
					 */
	/*
	 * The only exit from this loop used to be the chip reading back a
	 * clear status.  A card that stops answering the bus reads 0xff,
	 * and 0xff & ENISR_ALL is never zero, so the loop spun forever.
	 * That is fatal rather than merely wrong: this handler also runs
	 * from the receive-poll timer, i.e. inside a bottom half, and
	 * do_bottom_half() never returning means in_bottom_half stays set,
	 * no further bottom half ever runs, and schedule() -- which the IRQ
	 * return path reaches only after do_bottom_half() -- is never
	 * called again.  Every process stops dead, including whichever one
	 * was inside an INT 13h call to the serial root disk, which is why
	 * the machine died with its keyboard unresponsive.  Bound it.
	 */
	loops = ULTRA_INT_MAXLOOP;
	while (1) {
		stat = inb(ULTRA_8390_PORT + EN0_ISR);
		if (stat == 0xff)
			break;			/* card gone or floating bus */
		if (!(stat & ENISR_ALL))
			break;
		if (!--loops) {
			outb(0xff, ULTRA_8390_PORT + EN0_ISR);
			ultra_stat_inc(&netif_stat.rq_errors);
			break;
		}
		if (stat & ENISR_OFLOW) {
			if (verbose)
				printk(EMSG_OFLOW, dev_name, stat,
					netif_stat.oflow_keep);
			ultra_clr_oflow(netif_stat.oflow_keep);
			ultra_stat_inc(&netif_stat.oflow_errors);
			if (netif_stat.oflow_keep)
				wake_up(&rxwait);
			continue; /* Everything has been reset, skip rest of the loop */
		}
		if (stat & ENISR_RX) {
			wake_up(&rxwait);
			outb(ENISR_RX, ULTRA_8390_PORT + EN0_ISR);
		}
		if (stat & ENISR_TX) {
			wake_up(&txwait);
			inb(ULTRA_8390_PORT + EN0_TSR);
			outb(ENISR_TX, ULTRA_8390_PORT + EN0_ISR);
		}
		if (stat & ENISR_RX_ERR) {
			unsigned char rsr = inb(ULTRA_8390_PORT + EN0_RSR);

			ultra_stat_inc(&netif_stat.rx_errors);
			if (verbose)
				printk(EMSG_RXERR, dev_name, rsr);
			outb(ENISR_RX_ERR, ULTRA_8390_PORT + EN0_ISR);
		}
		if (stat & ENISR_TX_ERR) {
			unsigned char tsr = inb(ULTRA_8390_PORT + EN0_TSR);

			ultra_stat_inc(&netif_stat.tx_errors);
			if (verbose)
				printk(EMSG_TXERR, dev_name, tsr);
			outb(ENISR_TX_ERR, ULTRA_8390_PORT + EN0_ISR);
		}
		if (stat & ENISR_COUNTERS) {
			ultra_drain_err_counters();
			outb(ENISR_COUNTERS, ULTRA_8390_PORT + EN0_ISR);
		}
		if (stat & ENISR_RDC) {
			outb(ENISR_RDC, ULTRA_8390_PORT + EN0_ISR);
		}
	}
	outb(ENISR_ALL, ULTRA_8390_PORT + EN0_IMR);
			
}

/*
 * Ethernet main initialization (during boot)
 */

void INITPROC ultra_drv_init(void)
{
	unsigned u;
	word_t hw_addr[6];
	byte_t *mac_addr = (byte_t *)&netif_stat.mac_addr;

	if (!net_port) {
		printk("eth: %s ignored\n", dev_name);
		return;
	}
	printk("eth: %s at 0x%x, irq %d, ram 0x%x",
		dev_name, net_port, net_irq, net_ram);
	if (ultra_probe()) {
		printk(" not found\n");
	} else {
		found++;
		ultra_get_hw_addr(hw_addr);
		for (u = 0; u < 6; u++)
			mac_addr[u] = hw_addr[u]&0xff;
		if (model_is_ez) {
			model_name[4] = 'e';
			model_name[5] = 'z';
			model_name[6] = '\0';
		}
		printk(", (%s) MAC %02X", model_name, mac_addr[0]);
		for (u = 1; u < 6; u++)
			printk(":%02X", mac_addr[u]);
		/* net_irq/net_ram may have been resolved from the card above,
		 * so report the values actually in use, not the requested ones. */
		printk(", using irq %d ram 0x%x", net_irq, net_ram);
		if (verbose) printk(", type 0x%x", inb(net_port+0xe)&0xff);
		printk(", flags 0x%x\n", net_flags);
		eths[ETH_ULTRA].stats = &netif_stat;
	}
}

/* Keep Ultra shared-memory transfers byte-wide for XT-class ISA safety. */
static void NICPROC fmemcpy(void *dst_off, seg_t dst_seg, void *src_off, seg_t src_seg,
	size_t count, int type) {

	(void)type;
	fmemcpyb(dst_off, dst_seg, src_off, src_seg, count);
}
