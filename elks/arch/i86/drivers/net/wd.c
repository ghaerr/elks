/*
 *
 * WD Ethernet driver - supports NICs using the WD8003 and compatible
 * chip sets. Tested with SMC 8003WC ISA 8-bit card.
 *
 */

#include <stddef.h>
#include <arch/io.h>
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

/* I/O delay settings */
#define INB	inb	/* use inb_p for 1us delay */
#define OUTB	outb	/* use outb_p for 1us delay */

#define WD_SHMEMSEG	0xd000U /* RAM base address */

#define WD_STAT_RX	0x0001U	/* packet received */
#define WD_STAT_TX	0x0002U	/* packet sent */
#define WD_STAT_OF	0x0010U	/* RX ring overflow */

#define WD_START_PG	0x00U	/* First page of TX buffer */
#define WD_STOP_PG	0x20U	/* Last page + 1 of RX ring */

#define TX_2X_PAGES	12U
#define TX_1X_PAGES	6U
#define TX_PAGES	TX_1X_PAGES

#define WD_FIRST_TX_PG	WD_START_PG
#define WD_FIRST_RX_PG	(WD_FIRST_TX_PG + TX_PAGES)

#define RX_PAGES	(WD_STOP_PG - WD_FIRST_RX_PG)

#define WD_RESET	0x80U	/* Board reset */
#define WD_MEMENB	0x40U	/* Enable the shared memory */
#define WD_IO_EXTENT	32U
#define WD_8390_OFFSET	16U
#define WD_8390_PORT	(WD_PORT + WD_8390_OFFSET)

#define E8390_RXCONFIG	0x04U	/* EN0_RXCR: broadcasts, no multicast,errors */
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
#define ENISR_OVER	0x10U	/* Receiver overwrote the ring */
#define ENISR_COUNTERS	0x20U	/* Counters need emptying */
#define ENISR_RDC	0x40U	/* remote dma complete */
#define ENISR_RESET	0x80U	/* Reset completed */
#define ENISR_ALL	0x3fU	/* Interrupts we will enable */

typedef struct {
	unsigned char status;	/* status */
	unsigned char next;	/* pointer to next packet */
	unsigned short count;	/* header + packet length in bytes */
} __attribute__((packed)) e8390_pkt_hdr;

static struct wait_queue rxwait;
static struct wait_queue txwait;

static byte_t wd_inuse = 0U;
static byte_t mac_addr[6U];

static unsigned char current_rx_page = WD_FIRST_RX_PG;

static word_t wd_rx_stat(void);
static word_t wd_tx_stat(void);

/*
 * Get MAC
 */

static void wd_get_hw_addr(word_t * data)
{
	unsigned u;

	for (u = 0U; u < 6U; u++)
		data[u] = INB(WD_PORT + 8U + u);
}

/*
 * Reset
 */

static void wd_reset(void)
{
	OUTB(WD_RESET, WD_PORT);
	OUTB(((WD_SHMEMSEG >> 9U) & 0x3fU) | WD_MEMENB, WD_PORT);
}

/*
 * Init
 */

static void wd_init_8390(void)
{
	unsigned u;

	OUTB(E8390_NODMA | E8390_PAGE0 | E8390_STOP,
		WD_8390_PORT + E8390_CMD);
	OUTB(0x48U, WD_8390_PORT + EN0_DCFG); /* 0x48 vs 0x49: 8 vs 16 bit */
	/* Clear the remote byte count registers. */
	OUTB(0x00U, WD_8390_PORT + EN0_RCNTLO);
	OUTB(0x00U, WD_8390_PORT + EN0_RCNTHI);
	/* Set to monitor and loopback mode. */
	OUTB(E8390_RXOFF, WD_8390_PORT + EN0_RXCR);
	OUTB(E8390_TXOFF, WD_8390_PORT + EN0_TXCR);
	/* Set the transmit page and receive ring. */
	OUTB(WD_FIRST_TX_PG, WD_8390_PORT + EN0_TPSR);
	OUTB(WD_FIRST_RX_PG, WD_8390_PORT + EN0_STARTPG);
	OUTB(WD_STOP_PG - 1U, WD_8390_PORT + EN0_BOUNDARY);
	OUTB(WD_STOP_PG, WD_8390_PORT + EN0_STOPPG);
	/* Clear the pending interrupts and mask. */
	OUTB(0xffU, WD_8390_PORT + EN0_ISR);
	OUTB(0x00U, WD_8390_PORT + EN0_IMR);
	/* Copy the station address into the DS8390 registers. */
	clr_irq();
	OUTB(E8390_NODMA | E8390_PAGE1 | E8390_STOP,
		WD_8390_PORT + E8390_CMD);
	for(u = 0U; u < 6U; u++)
		OUTB(mac_addr[u], WD_8390_PORT + EN1_PHYS + u);
	OUTB(WD_FIRST_RX_PG, WD_8390_PORT + EN1_CURPAG);
	OUTB(E8390_NODMA | E8390_PAGE0 | E8390_STOP,
		WD_8390_PORT + E8390_CMD);
	set_irq();
}

static void wd_init(void)
{
	wd_init_8390();
	OUTB(0xffU, WD_8390_PORT + EN0_ISR);
	OUTB(ENISR_ALL, WD_8390_PORT + EN0_IMR);
	OUTB(E8390_NODMA | E8390_PAGE0 | E8390_START,
		WD_8390_PORT + E8390_CMD);
	OUTB(E8390_TXCONFIG, WD_8390_PORT + EN0_TXCR); /* xmit on */
	/* Only after the NIC is started: */
	OUTB(E8390_RXCONFIG, WD_8390_PORT + EN0_RXCR); /* rx on */
}

/*
 * Start
 */

static void wd_start(void)
{
	OUTB(((WD_SHMEMSEG >> 9U) & 0x3fU) | WD_MEMENB, WD_PORT);
}

/*
 * Stop
 */

static void wd_stop(void)
{
	OUTB(((WD_SHMEMSEG >> 9U) & 0x3fU) & ~WD_MEMENB, WD_PORT);
}

/*
 * Termination
 */

static void wd_term(void)
{
	wd_init_8390(); /* SIC! */
}

/*
 * Get packet
 */

static int wd_pack_get(char *data, size_t len)
{
	const e8390_pkt_hdr __far *rxhdr;
	word_t hdr_start;
	unsigned char this_frame;
	int res = -EIO;

	do {
		/* Remove one frame from the ring. */
		/* Boundary is always a page behind. */
		this_frame = INB(WD_8390_PORT + EN0_BOUNDARY) + 1U;
		if (this_frame >= WD_STOP_PG)
			this_frame = WD_FIRST_RX_PG;
		if (this_frame != current_rx_page)
			debug_eth("eth: mismatched read page pointers %2x vs %2x.\n",
				this_frame, current_rx_page);
		hdr_start = (this_frame - WD_START_PG) << 8U;
		rxhdr = _MK_FP(WD_SHMEMSEG, hdr_start);
		if ((rxhdr->count < 64U) ||
		    (rxhdr->count > (MAX_PACKET_ETH + sizeof(e8390_pkt_hdr)))) {
			debug_eth("eth: bogus packet size: %d, "
				"status = %#2x nxpg = %#2x.\n",
				rxhdr->count, rxhdr->status, rxhdr->next);
			break;
		}
		current_rx_page = rxhdr->next;
		if ((rxhdr->status & 0x0fU) != ENRSR_RXOK) {
			debug_eth("eth: bogus packet: "
				"status = %#2x nxpg = %#2x size = %d\n",
				rxhdr->status, rxhdr->next, rxhdr->count);
			break;
		} 
		res = rxhdr->count - sizeof(e8390_pkt_hdr);
		if (res > len) res = len;
		if (current_rx_page > this_frame || current_rx_page == WD_FIRST_RX_PG) {
			/* no wrap around */
			fmemcpyb(data, current->t_regs.ds,
				(char *)hdr_start + sizeof(e8390_pkt_hdr), WD_SHMEMSEG, res);
		} else {	/* handle wrap-around */
			size_t len1 = ((WD_STOP_PG - this_frame) << 8) - sizeof(e8390_pkt_hdr);
                        fmemcpyb(data, current->t_regs.ds,
                                (char *)hdr_start + sizeof(e8390_pkt_hdr), WD_SHMEMSEG, len1);
                        fmemcpyb(data+len1, current->t_regs.ds,
                                (char *)(WD_FIRST_RX_PG << 8), WD_SHMEMSEG, res-len1);
 		}
	} while (0);
	/* this is not always strictly correct but apparently works */
	OUTB(current_rx_page - 1U, WD_8390_PORT + EN0_BOUNDARY);
	return res;
}

static size_t wd_read(struct inode * inode, struct file * filp,
	char * data, size_t len)
{
	int res = 0;

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
		res = wd_pack_get(data, len);	/* returns packet data size read*/
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
		fmemcpyb((byte_t *)((WD_FIRST_TX_PG - WD_START_PG) << 8U),
			WD_SHMEMSEG, data, current->t_regs.ds, len);
		OUTB(E8390_NODMA | E8390_PAGE0, WD_8390_PORT + E8390_CMD);
		if (INB(WD_8390_PORT + E8390_CMD) & E8390_TRANS) {
			printk("eth: attempted send with the tr busy.\n");
			len = -EIO;
			break;
		}
		OUTB(len & 0xffU, WD_8390_PORT + EN0_TCNTLO);
		OUTB(len >> 8U, WD_8390_PORT + EN0_TCNTHI);
		OUTB(WD_FIRST_TX_PG, WD_8390_PORT + EN0_TPSR);
		OUTB(E8390_NODMA | E8390_TRANS | E8390_START,
			WD_8390_PORT + E8390_CMD);
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
 * Clear overflow
 */

static void wd_clr_oflow(void)
{
	printk("eth: Buffer overflow, resetting.\n");
	wd_stop();
	wd_term();
	current_rx_page = WD_FIRST_RX_PG;
	wd_reset();
	wd_init();
	if (INB(WD_PORT + 14U) & 0x20U) /* enable IRQ on softcfg card */
		OUTB(INB(WD_PORT + 4U) | 0x80U, WD_PORT + 4U);
	wd_start();
}

/*
 * Test for readiness
 */

static word_t wd_rx_stat(void)
{
	unsigned char rxing_page;

	clr_irq();
	/* Get the rx page (incoming packet pointer). */
	OUTB(E8390_NODMA | E8390_PAGE1, WD_8390_PORT + E8390_CMD);
	rxing_page = INB(WD_8390_PORT + EN1_CURPAG);
	OUTB(E8390_NODMA | E8390_PAGE0, WD_8390_PORT + E8390_CMD);
	set_irq();
	return (current_rx_page == rxing_page) ? 0U : WD_STAT_RX;
}

static word_t wd_tx_stat(void)
{
	return (INB(WD_8390_PORT + E8390_CMD) & E8390_TRANS) ? 0U :
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
		memcpy_tofs((char *)arg, mac_addr, 6U);
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
		wd_get_hw_addr((word_t *)arg);
		break;
	case IOCTL_ETH_GETSTAT:
		/* Get statistics from the NIC hardware (error counts etc.)
		 * arg is byte[3].
		 */
		err = -ENOSYS;
		break;
	case IOCTL_ETH_OFWSKIP_SET:
		/* Set the number of packets to skip @ ring buffer overflow. */
		err = -ENOSYS;
		break;
	case IOCTL_ETH_OFWSKIP_GET:
		/* Get the current overflow skip counter. */
		err = -ENOSYS;
		break;
#endif
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
		if (wd_inuse) {
			err = -EBUSY;
			break;
		}
		wd_reset();
		wd_init();
		if (INB(WD_PORT + 14U) & 0x20U) /* enable IRQ on softcfg card */
			OUTB(INB(WD_PORT + 4U) | 0x80U, WD_PORT + 4U);
		wd_start();
		wd_inuse = 1U;
	} while (0);
	return err;
}

/*
 * Release (close) device
 */

static void wd_release(struct inode * inode, struct file * file)
{
	wd_stop();
	wd_term();
	wd_inuse = 0U;
}

/*
 * Ethernet operations
 */

static struct file_operations wd_fops =
{
	NULL,         /* lseek */
	wd_read,
	wd_write,
	NULL,         /* readdir */
	wd_select,
	wd_ioctl,
	wd_open,
	wd_release,
#ifdef BLOAT_FS
	NULL,         /* fsync */
	NULL,         /* check_media_type */
	NULL,         /* revalidate */
#endif
};

/*
 * Interrupt handler
 */

static word_t wd_int_stat(void)
{
	byte_t interrupts;
	word_t retval = 0U;

	/* Change to page 0 and read the intr status reg. */
	OUTB(E8390_NODMA | E8390_PAGE0, WD_8390_PORT + E8390_CMD);
	interrupts = INB(WD_8390_PORT + EN0_ISR);
	if (interrupts & ENISR_RX)
		retval |= WD_STAT_RX;
	if (interrupts & ENISR_TX)
		retval |= WD_STAT_TX;
	if (interrupts & ENISR_OVER)
		retval |= WD_STAT_OF;
	else if (interrupts)
		OUTB(0xffU, WD_8390_PORT + EN0_ISR); /* ack ALL */
	OUTB(E8390_NODMA | E8390_PAGE0 | E8390_START,
		WD_8390_PORT + E8390_CMD);
	return retval;
}

static void wd_int(int irq, struct pt_regs * regs, void * dev_id)
{
	word_t stat;

	stat = wd_int_stat();
	debug_eth("/%02X", stat & 0xffU);
	if (stat & WD_STAT_OF)
		wd_clr_oflow();
	if (stat & WD_STAT_RX)
		wake_up(&rxwait);
	if (stat & WD_STAT_TX)
		wake_up(&txwait);
}

/*
 * Ethernet main initialization (during boot)
 */

void wd_drv_init(void)
{
	int err;
	unsigned u;
	word_t hw_addr[6U];

	do {
		err = request_irq(WD_IRQ, wd_int, NULL);
		if (err) {
			printk("eth: WD IRQ %d request error: %i\n",
				WD_IRQ, err);
			break;
		}
		err = register_chrdev(ETH_MAJOR, "eth", &wd_fops);
		if (err) {
			printk("eth: register error: %i\n", err);
			break;
		}
		wd_get_hw_addr(hw_addr);
		for (u = 0U; u < 6U; u++)
			mac_addr[u] = (hw_addr[u] & 0xffU);
		printk ("eth: SMC/WD8003 at 0x%x, irq %d, MAC %02X",
			WD_PORT, WD_IRQ, mac_addr[0]);
		for (u = 1U; u < 6U; u++)
			printk(":%02X", mac_addr[u]);
		printk("\n");
	} while (0);
}
