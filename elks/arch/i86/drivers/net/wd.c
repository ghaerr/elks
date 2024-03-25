/*
 *
 * SMC/WD 80x3 (aka 'Elite') Ethernet driver for ELKS - supports wd8003 (8bit)
 * and wd8013 (16 bit) in numerous variants, with (assumed) reliable detection
 * of 8 vs 16 bits.
 *
 * By @pawosm-arm September 2020. 16 bit support and various other enhancements by 
 * Helge Skrivervik (@mellvik) june 2022.
 */

#include <arch/io.h>
#include <arch/irq.h>
#include <arch/ports.h>
#include <arch/segment.h>
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
#include "eth-msgs.h"

/* runtime configuration set in /bootopts or defaults in ports.h */
#define net_irq		(netif_parms[ETH_WD].irq)
#define net_port	(netif_parms[ETH_WD].port)
#define net_ram		(netif_parms[ETH_WD].ram)
#define net_flags	(netif_parms[ETH_WD].flags)

#define WD_STAT_RX	0x0001U	/* packet received */
#define WD_STAT_TX	0x0002U	/* packet sent */
#define WD_STAT_OF	0x0010U	/* RX ring overflow */

#define WD_START_PG	0x00U	/* First page of TX buffer */
#define WD_STOP_PG4	0x10U	/* Only used for arithmetic */
#define WD_STOP_PG8	0x20U	/* Last page + 1 of RX ring if 8 bit i/f */
#define WD_STOP_PG16	0x40U	/* Last page + 1 of RX ring if 16bit i/f */
#define WD_STOP_PG32	0x80U	/* Last page + 1 of RX ring if 32K buf space */

#define TX_2X_PAGES	12U
#define TX_1X_PAGES	6U
#define TX_PAGES	TX_1X_PAGES

#define WD_FIRST_TX_PG	WD_START_PG
#define WD_FIRST_RX_PG	(WD_FIRST_TX_PG + TX_PAGES)

#define WD_RESET	0x80U	/* Board reset */
#define WD_MEMENB	0x40U	/* Enable the shared memory */
#define WD_IO_EXTENT	32U
#define WD_8390_OFFSET	16U
#define WD_8390_PORT	(net_port + WD_8390_OFFSET)
#define WD_CMDREG5	5	/* Offset to 16-bit-only ASIC register 5. */

#define	ISA16		0x80	/* Enable 16 bit access from the ISA bus. */
#define	NIC16		0x40	/* Enable 16 bit access from the 8390. */

#define E8390_RXCONFIG	0x04U	/* EN0_RXCR: broadcasts, no multicast or errors */
#define E8390_RXOFF	0x20U	/* EN0_RXCR: Accept no packets */
#define E8390_TXCONFIG	0x00U	/* EN0_TXCR: Normal transmit mode */
#define E8390_TXOFF	0x02U	/* EN0_TXCR: Transmitter off */
#define E8390_STOP	0x01U	/* Stop and reset the chip */
#define E8390_START	0x02U	/* Start the chip, clear reset */
#define E8390_TRANS	0x04U	/* Transmit a frame */
#define E8390_RREAD	0x08U	/* Remote read */
#define E8390_RWRITE	0x10U	/* Remote write  */
#define E8390_NODMA	0x20U	/* Remote DMA */
#define E8390_PAGE0	0x00U	/* Select page chip registers */
#define E8390_PAGE1	0x40U	/* using the two high-order bits */
#define E8390_PAGE2	0x80U	/* Page 3 is invalid */
#define E8390_CMD	0x00U	/* The command register (for all pages) */

/* For register EN0_ISR. */
#define E8390_TX_IRQ_MASK	0x0aU
#define E8390_RX_IRQ_MASK	0x05U

/* Page 0 register offsets. */
#define EN0_CLDALO	0x01U	/* Low byte of current local dma addr  RD */
#define EN0_STARTPG	0x01U	/* Starting page of ring bfr WR */
#define EN0_CLDAHI	0x02U	/* High byte of current local dma addr  RD */
#define EN0_STOPPG	0x02U	/* Ending page +1 of ring bfr WR */
#define EN0_BOUNDARY	0x03U	/* Boundary page of ring bfr RD WR */
#define EN0_TSR		0x04U	/* Transmit status reg RD */
#define EN0_TPSR	0x04U	/* Transmit starting page WR */
#define EN0_NCR		0x05U	/* Number of collision reg RD */
#define EN0_TCNTLO	0x05U	/* Low  byte of tx byte count WR */
#define EN0_FIFO	0x06U	/* FIFO RD */
#define EN0_TCNTHI	0x06U	/* High byte of tx byte count WR */
#define EN0_ISR		0x07U	/* Interrupt status reg RD WR */
#define EN0_CRDALO	0x08U	/* low byte of current remote dma address RD */
#define EN0_RSARLO	0x08U	/* Remote start address reg 0 */
#define EN0_CRDAHI	0x09U	/* high byte, current remote dma address RD */
#define EN0_RSARHI	0x09U	/* Remote start address reg 1 */
#define EN0_RCNTLO	0x0aU	/* Remote byte count reg WR */
#define EN0_RCNTHI	0x0bU	/* Remote byte count reg WR */
#define EN0_RSR		0x0cU	/* rx status reg RD */
#define EN0_RXCR	0x0cU	/* RX configuration reg WR */
#define EN0_TXCR	0x0dU	/* TX configuration reg WR */
#define EN0_COUNTER0	0x0dU	/* Rcv alignment error counter RD */
#define EN0_DCFG	0x0eU	/* Data configuration reg WR */
#define EN0_COUNTER1	0x0eU	/* Rcv CRC error counter RD */
#define EN0_IMR		0x0fU	/* Interrupt mask reg WR */
#define EN0_COUNTER2	0x0fU	/* Rcv missed frame error counter RD */

/* Page 1 register offsets. */
#define EN1_PHYS	0x01U	/* This board's physical enet addr RD WR */
#define EN1_CURPAG	0x07U	/* Current memory page RD WR */
#define EN1_MULT	0x08U	/* Multicast filter mask array (8 bytes) RDWR */

/* Bits in received packet status byte and EN0_RSR. */
#define ENRSR_RXOK	0x01U	/* Received a good packet */
#define ENRSR_CRC	0x02U	/* CRC error */
#define ENRSR_FAE	0x04U	/* frame alignment error */
#define ENRSR_FO	0x08U	/* FIFO overrun */
#define ENRSR_MPA	0x10U	/* missed pkt */
#define ENRSR_PHY	0x20U	/* physical/multicase address */
#define ENRSR_DIS	0x40U	/* receiver disable. set in monitor mode */
#define ENRSR_DEF	0x80U	/* deferring */

/* Bits in EN0_ISR - Interrupt status register. */
#define ENISR_RX	0x01U	/* Receiver, no error */
#define ENISR_TX	0x02U	/* Transmitter, no error */
#define ENISR_RX_ERR	0x04U	/* Receiver, with error */
#define ENISR_TX_ERR	0x08U	/* Transmitter, with error */
#define ENISR_OFLOW	0x10U	/* Receiver overwrote the ring */
#define ENISR_COUNTERS	0x20U	/* Counters need emptying */
#define ENISR_RDC	0x40U	/* remote dma complete */
#define ENISR_RESET	0x80U	/* Reset completed */
#define ENISR_ALL	0x1fU	/* Enable these interrupts, skip RDC and stats */


typedef struct {
	unsigned char status;	/* status */
	unsigned char next;	/* pointer to next packet */
	unsigned short count;	/* header + packet length in bytes */
} __attribute__((packed)) e8390_pkt_hdr;

static struct wait_queue rxwait;
static struct wait_queue txwait;

static byte_t usecount;
static byte_t is_8bit;
static byte_t model_name[] = "wd80x3";
static byte_t dev_name[] = "wd0";
static byte_t stop_page; 	/* actual last pg of ring (+1) */
static unsigned char found;
static unsigned int verbose;

static unsigned char current_rx_page;
static struct netif_stat netif_stat;

static word_t wd_rx_stat(void);
static word_t wd_tx_stat(void);
static void wd_int(int irq, struct pt_regs * regs);
static void fmemcpy(void *, seg_t, void *, seg_t, size_t, int);

extern struct eth eths[];

/*
 * Get MAC
 */

static void wd_get_hw_addr(word_t * data)
{
	unsigned u;

	for (u = 0; u < 6; u++)
		data[u] = inb(net_port + 8 + u);
}

/*
 * Check if card exists, get MAC addr from PROM,
 * determine bus width if possible.
 */

static int INITPROC wd_probe(void) {
	int i, tmp = 0;

	for (i = 0; i < 8; i++)
		tmp += inb(net_port + 8 + i);
	if (inb(net_port + 8) == 0xff	/* Extra check to avoid soundcard. */
		|| inb(net_port + 9) == 0xff
		|| (tmp & 0xff) != 0xFF)
		return -ENODEV;	

	/*  device found - check what type */
	tmp = inb(net_port+1);			/* fiddle with 16bit bit */
	outb(tmp ^ 0x01, net_port+1 );		/* attempt to clear 16bit bit */
	if (((inb(net_port+1) & 0x01) == 0x01)	/* A 16 bit card */
				&& (tmp & 0x01) == 0x01	) {		/* In a 16 slot. */
		int asic_reg5 = inb(net_port+WD_CMDREG5);
		/* Magic to set ASIC to word-wide mode. */
		outb(NIC16 | (asic_reg5&0x1f), net_port+WD_CMDREG5);
		outb(tmp, net_port+1);
		model_name[4] = '1';
		is_8bit = 0;		/* We have a 16bit board here! */
		stop_page = WD_STOP_PG16;	// FIXME: calculate from config parameter
						// since some interfaces have large (32k)
						// buffer ram
		netif_stat.oflow_keep = 3;
	} else {
		model_name[4] = '0';	// FIXME: Incorrect if 16bit i/f in 8bit slot */
		is_8bit = 1;		/* board must be 8 bit */
		if (!(net_flags & (ETHF_8BIT_BUS | ETHF_16BIT_BUS)))
			 netif_stat.if_status |= NETIF_AUTO_8BIT;
		netif_stat.oflow_keep = 1;
		stop_page = WD_STOP_PG8;
	}
	/* do the config flag processing */
	if (net_flags&ETHF_8BIT_BUS)  is_8bit = 1;
	if (net_flags&ETHF_16BIT_BUS) is_8bit = 0;	/* overrides 8bit */
	if (net_flags&0x07)				/* Force buffer size */
		stop_page = WD_STOP_PG4 << (net_flags&0x03);
	verbose = (net_flags&ETHF_VERBOSE);	/* set verbose messages */
	//printk("net_flags %04x stop_page %02x verbose %d\n", net_flags, stop_page, verbose);
	outb(tmp, net_port+1);			/* Restore original reg1 value. */

	return 0;
}

/*
 * Reset
 */

static void wd_reset(void)
{
	outb(WD_RESET, net_port);
	// FIXME: should wait until reset has completed, works OK on slow machines.
	outb(((net_ram >> 9U) & 0x3fU) | WD_MEMENB, net_port);
}

/*
 * Init	- basic 8390 initialization. If the strategy parameter is 0, do complete
 *	  initialization, otherwise just recover from a receive overflow and 
 *	  keep 'strategy' # of packets in the buffer.
 */

static void wd_init_8390(int strategy)
{
	unsigned u;
	const e8390_pkt_hdr __far *rxhdr;
	word_t hdr_start;
	byte_t *mac_addr = (byte_t *)&netif_stat.mac_addr;

	outb(E8390_NODMA | E8390_PAGE0 | E8390_STOP,
		WD_8390_PORT + E8390_CMD);
	outb(0x49 - is_8bit, WD_8390_PORT + EN0_DCFG);	/* 0x48 vs 0x49: 8 vs 16 bit */

	/* Clear the remote byte count registers. */
	outb(0x00, WD_8390_PORT + EN0_RCNTLO);
	outb(0x00, WD_8390_PORT + EN0_RCNTHI);

	/* Set to monitor and loopback mode. */
	outb(E8390_RXOFF, WD_8390_PORT + EN0_RXCR);
	outb(E8390_TXOFF, WD_8390_PORT + EN0_TXCR);

	/* Clear the pending interrupts and mask. */
	outb(0xffU, WD_8390_PORT + EN0_ISR);
	outb(0x00U, WD_8390_PORT + EN0_IMR);

	if (strategy == 0) {	/* full initialization */
		/* Set the transmit page and receive ring. */
		outb(WD_FIRST_TX_PG, WD_8390_PORT + EN0_TPSR);
		outb(WD_FIRST_RX_PG, WD_8390_PORT + EN0_STARTPG);
		outb(stop_page, WD_8390_PORT + EN0_STOPPG);
		current_rx_page = WD_FIRST_RX_PG;
		outb(stop_page - 1, WD_8390_PORT + EN0_BOUNDARY);


		/* Copy the station address into the DS8390 registers. */
		clr_irq();
		outb(E8390_NODMA | E8390_PAGE1 | E8390_STOP,
			WD_8390_PORT + E8390_CMD);
		for (u = 0U; u < 6U; u++)
			outb(mac_addr[u], WD_8390_PORT + EN1_PHYS + u);
		outb(WD_FIRST_RX_PG, WD_8390_PORT + EN1_CURPAG);
		outb(E8390_NODMA | E8390_PAGE0 | E8390_STOP,
			WD_8390_PORT + E8390_CMD);
		set_irq();

	} else {	/* 'strategy' is the # of packets to keep. */
			/* This is for overflow recovery */
		hdr_start = (current_rx_page - WD_START_PG) << 8U;
		rxhdr = _MK_FP(net_ram, hdr_start);

		// FIXME: currently retains only one packet regardless 
		// of the value of strategy
		//printk("front: %x end: %x-", rxhdr->next, current_rx_page);
		outb(E8390_NODMA | E8390_PAGE1 | E8390_STOP,
			WD_8390_PORT + E8390_CMD);
		outb(rxhdr->next, WD_8390_PORT + EN1_CURPAG);
		outb(E8390_NODMA | E8390_PAGE0 | E8390_STOP,
			WD_8390_PORT + E8390_CMD);
	}

}

/*
 * Start
 */

static void wd_start(void)
{
	if (inb(net_port + 14U) & 0x20U) /* enable IRQ on softcfg card */
		outb(inb(net_port + 4U) | 0x80U, net_port + 4U);
	outb(((net_ram >> 9U) & 0x3fU) | WD_MEMENB, net_port);

	outb(E8390_TXCONFIG, WD_8390_PORT + EN0_TXCR); /* xmit on */
	outb(E8390_RXCONFIG, WD_8390_PORT + EN0_RXCR); /* rx on */

	outb(E8390_NODMA | E8390_PAGE0 | E8390_START, WD_8390_PORT + E8390_CMD);
	outb(ENISR_ALL, WD_8390_PORT + EN0_IMR);	/* enable interrupts */
}

/*
 * Stop & terminate
 */

static void wd_stop(void)
{
	outb(((net_ram >> 9U) & 0x3fU) & ~WD_MEMENB, net_port);
	outb(0, WD_8390_PORT + EN0_IMR);	/* disable interrupts */
}

/*
 * Clear overflow
 *
 *	For ELKS, when an overflow occurs, the kernel will probably just have
 *	received a wakeup() from a RxComplete interrupt. 
 *	If the overflow handler purges the receive buffer
 *	completely, the next read will fail - there is nothing to read. No big
 *	deal, but noisy (error messages) and inefficient since the buffer is at
 *	least 8k. So we let 1 or more packets survive the overflow recovery (as
 *	specified by the parameter to wd_init_8390() ).
 */

static void wd_clr_oflow(int keep)
{
	wd_init_8390(keep);
	wd_start();
}

/*
 * Get packet
 */

static size_t wd_pack_get(char *data, size_t len)
{
	const e8390_pkt_hdr __far *rxhdr;
	word_t hdr_start;
	unsigned char this_frame, update = 1;
	size_t res = -EIO;

	//clr_irq();	// EXPERIMENTAL
	outb(0x00U, WD_8390_PORT + EN0_IMR);	// Block interrupts
	do {
		/* Remove one frame from the ring. */
		/* Boundary is always a page behind. */
		this_frame = inb(WD_8390_PORT + EN0_BOUNDARY) + 1U;
		if (this_frame >= stop_page)
			this_frame = WD_FIRST_RX_PG;
		if (this_frame != current_rx_page)	/* Very useful for debugging ! */
			printk("eth: mismatched read page pointers %2x vs %2x.\n",
				this_frame, current_rx_page);
		hdr_start = (this_frame - WD_START_PG) << 8U;
		rxhdr = _MK_FP(net_ram, hdr_start);

		if ((rxhdr->count < 64U) ||
		    (rxhdr->count > (MAX_PACKET_ETH + sizeof(e8390_pkt_hdr)))) {

			/* This should not happen! The NIC is programmed to drop
			 * erroneous packets. If we get here, it's most likely
			 * a driver bug or a hardware problem. */
			/* If this happens, we need to purge the NIC buffer entirely 
			 * since if the size is bogus, the next packet pointer is
			 * unreliable at best. */
			netif_stat.rq_errors++;
			if (verbose) printk(EMSG_DMGPKT, dev_name, (unsigned int *)rxhdr, rxhdr->next);
			
			wd_clr_oflow(0);	/* Complete reset */
			update = 0;		/* exit flag */
			break;
		}
		current_rx_page = rxhdr->next;
		if ((rxhdr->status & 0x0fU) != ENRSR_RXOK) {
			/* This shouldn't happen either, see comment above */
			netif_stat.rx_errors++;
			if (verbose) printk(EMSG_BGSPKT, dev_name,
				rxhdr->status, rxhdr->next, rxhdr->count);
			break;
		} 
		res = rxhdr->count - sizeof(e8390_pkt_hdr);
		if (res > len) res = len;
		if (current_rx_page > this_frame || current_rx_page == WD_FIRST_RX_PG) {
			/* no wrap around */
			fmemcpy(data, current->t_regs.ds,
				(char *)hdr_start + sizeof(e8390_pkt_hdr), net_ram, res, is_8bit);
		} else {	/* handle wrap-around */
			size_t len1 = ((stop_page - this_frame) << 8) - sizeof(e8390_pkt_hdr);
			fmemcpy(data, current->t_regs.ds,
				(char *)hdr_start + sizeof(e8390_pkt_hdr), net_ram, len1, is_8bit);
			fmemcpy(data+len1, current->t_regs.ds,
				(char *)(WD_FIRST_RX_PG << 8), net_ram, res-len1, is_8bit);
 		}
	} while (0);

	if (update) {	/* don't update if we ran overflow recovery */
		this_frame = (current_rx_page == WD_FIRST_RX_PG) ? stop_page - 1 : current_rx_page - 1;
		outb(this_frame, WD_8390_PORT + EN0_BOUNDARY);
	}
	//set_irq();
	outb(ENISR_ALL, WD_8390_PORT + EN0_IMR);

	return res;
}

static size_t wd_read(struct inode * inode, struct file * filp,
	char * data, size_t len)
{
	size_t res = 0;

	do {
		prepare_to_wait_interruptible(&rxwait);
		if (wd_rx_stat() != WD_STAT_RX) {
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
		res = wd_pack_get(data, len);	/* returns packet data size read */
	} while (0);

	finish_wait(&rxwait);
	return res;
}

/*
 * Pass packet to driver for send
 */

static size_t wd_pack_put(char *data, size_t len)
{
	do {
		if (len > MAX_PACKET_ETH)
			len = MAX_PACKET_ETH;
		if (len < 64U) len = 64U;  /* issue #133 */

		fmemcpy((byte_t *)((WD_FIRST_TX_PG - WD_START_PG) << 8U),
			net_ram, data, current->t_regs.ds, len, is_8bit);
		outb(E8390_NODMA | E8390_PAGE0, WD_8390_PORT + E8390_CMD);

#if REMOVE
		/* FIXME: superfluous. Cannot get here unless we have a trans complete intr */
		/* which means the NIC is ready for more */
		if (inb(WD_8390_PORT + E8390_CMD) & E8390_TRANS) {
			printk("eth: attempted send with the tr busy.\n");
			len = -EIO;
			break;
		}
#endif
		outb(len & 0xffU, WD_8390_PORT + EN0_TCNTLO);
		outb(len >> 8U, WD_8390_PORT + EN0_TCNTHI);
		outb(WD_FIRST_TX_PG, WD_8390_PORT + EN0_TPSR);
		outb(E8390_NODMA | E8390_TRANS, WD_8390_PORT + E8390_CMD);
	} while (0);
	return len;
}

static size_t wd_write(struct inode * inode, struct file * file,
	char * data, size_t len)
{
	int res;

	do {
		prepare_to_wait_interruptible(&txwait);
		if (wd_tx_stat() != WD_STAT_TX) {
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
		res = wd_pack_put(data, len);
	} while (0);
	finish_wait(&txwait);
	return res;
}

/*
 * Test for readiness
 */

static word_t wd_rx_stat(void)
{
	unsigned char rxing_page;

	clr_irq();	// FIXME: don't need this, just block
			// our own interrupts
	/* Get the rx page (incoming packet pointer). */
	outb(E8390_NODMA | E8390_PAGE1, WD_8390_PORT + E8390_CMD);
	rxing_page = inb(WD_8390_PORT + EN1_CURPAG);
	outb(E8390_NODMA | E8390_PAGE0, WD_8390_PORT + E8390_CMD);
	set_irq();

	return (current_rx_page == rxing_page) ? 0U : WD_STAT_RX;
}

static word_t wd_tx_stat(void)
{
	return (inb(WD_8390_PORT + E8390_CMD) & E8390_TRANS) ? 0U :
		WD_STAT_TX;
}

static int wd_select(struct inode * inode, struct file * filp, int sel_type)
{
	int res = 0;

	switch (sel_type) {
		case SEL_OUT:
			if (wd_tx_stat() != WD_STAT_TX) {
				select_wait(&txwait);
				break;
			}
			res = 1;
			break;
		case SEL_IN:
			if (wd_rx_stat() != WD_STAT_RX) {
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

/*
 * I/O control
 */

static int wd_ioctl(struct inode * inode, struct file * file,
	unsigned int cmd, unsigned int arg)
{
	int err = 0;

	switch (cmd) {
	case IOCTL_ETH_ADDR_GET:
		err = verified_memcpy_tofs((char *)arg, &netif_stat.mac_addr, 6U);
		break;

#if UNUSED
	case IOCTL_ETH_ADDR_SET:
		err = -ENOSYS;
		break;

	case IOCTL_ETH_HWADDR_GET:
		/* Get the hardware address of the NIC,	which may be different
		 * from the currently programmed address. Be careful with this,
		 * it may interrupt ongoing send/receives.
		 * arg must be a 6 word array.
		 */
		wd_get_hw_addr((word_t *)arg);
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

static int wd_open(struct inode * inode, struct file * file)
{
	int err = 0;

	do {
		if (!found) {
			err = -ENODEV;
			break;
		}
		if (usecount++) 
			break;
		err = request_irq(net_irq, wd_int, INT_GENERIC);
		if (err) {
			printk(EMSG_IRQERR, dev_name, net_irq, err);
			break;
		}
		wd_init_8390(0);
		wd_start();
	} while (0);
	return err;
}

/*
 * Release (close) device
 */

static void wd_release(struct inode * inode, struct file * file)
{
	if (--usecount == 0) {
		wd_stop();
		wd_reset();
		free_irq(net_irq);
	}
}

/*
 * Ethernet operations
 */

struct file_operations wd_fops =
{
	NULL,         /* lseek */
	wd_read,
	wd_write,
	NULL,         /* readdir */
	wd_select,
	wd_ioctl,
	wd_open,
	wd_release
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

static void wd_int(int irq, struct pt_regs * regs)
{
	word_t stat;

	outb(0, WD_8390_PORT + EN0_IMR);/* Block interrupts,
					 * should not be required since the IRQ line
					 * is held high until all unmasked bits have
					 * been cleared. This is experimental.
					 */
	while (1) {
		stat = inb(WD_8390_PORT + EN0_ISR);
		//printk("/%02x;", stat&0xff);
		if (!(stat & ENISR_ALL))
			break;
		if (stat & ENISR_OFLOW) {
			printk(EMSG_OFLOW, dev_name, stat, netif_stat.oflow_keep);
			wd_clr_oflow(netif_stat.oflow_keep);
			netif_stat.oflow_errors++;
			continue; /* Everything has been reset, skip rest of the loop */
		}
		if (stat & ENISR_RX) {
			wake_up(&rxwait);
			outb(ENISR_RX, WD_8390_PORT + EN0_ISR);
		}
		if (stat & ENISR_TX) {
			wake_up(&txwait);
			//inb(WD_8390_PORT + EN0_TSR);	/* should be read every time */
			outb(ENISR_TX, WD_8390_PORT + EN0_ISR);
		}
		if (stat & ENISR_RX_ERR) {
			printk(EMSG_RXERR, dev_name, inb(WD_8390_PORT + EN0_RSR));
			netif_stat.rx_errors++;
			outb(ENISR_RX_ERR, WD_8390_PORT + EN0_ISR);
		}
		if (stat & ENISR_TX_ERR) {
			netif_stat.tx_errors++;
			printk(EMSG_TXERR, dev_name, inb(WD_8390_PORT + EN0_TSR));
			outb(ENISR_TX_ERR, WD_8390_PORT + EN0_ISR);
		}
		if (stat & (ENISR_RDC|ENISR_COUNTERS)) {  /* Remaining bits - should not happen */
			// FIXME: Need to add handling of the statistics registers
			printk("eth: RDC/Stat error, status 0x%x\n", stat & 0xe0);
			outb(ENISR_RDC|ENISR_COUNTERS, WD_8390_PORT + EN0_ISR);
		}
	}
	outb(ENISR_ALL, WD_8390_PORT + EN0_IMR);
			
}

/*
 * Ethernet main initialization (during boot)
 */

void INITPROC wd_drv_init(void)
{
	unsigned u;
	word_t hw_addr[6U];
	byte_t *mac_addr = (byte_t *)&netif_stat.mac_addr;

	u = wd_probe();
	printk("eth: %s at %x, irq %d, ram %04x",
		dev_name, net_port, net_irq, net_ram);
	if (u) {
		printk(" not found\n");
	} else {
		found++;
		wd_get_hw_addr(hw_addr);
		for (u = 0; u < 6; u++) 
			mac_addr[u] = hw_addr[u]&0xffU;
		printk(", (%s) MAC %02X", model_name, mac_addr[0]);
		for (u = 1U; u < 6U; u++) 
			printk(":%02X", mac_addr[u]);
		printk(", flags 0x%x\n", net_flags);
	}
	eths[ETH_WD].stats = &netif_stat;
	return;
}

/* Using this wrapper saves 44 bytes of RAM */
/* Using word transfers when possible improves transfer time ~10%
 * on large packets (measured @ 1200 bytes) */
static void fmemcpy(void *dst_off, seg_t dst_seg, void *src_off, seg_t src_seg, size_t count, int type) {

	if (type == is_8bit)
		fmemcpyb(dst_off, dst_seg, src_off, src_seg, count);
	else
		fmemcpyw(dst_off, dst_seg, src_off, src_seg, (count+1)>>1);
}
