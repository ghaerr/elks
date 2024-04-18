/*
	Driver for 3Com Etherlink III/3C5x9 network interface.
	For ELKS and 3C509 by Helge Skrivervik (@mellvik) may/june 2022

	Adapted from Linux driver by Donald Becker et al.

	Parts Copyright 1994-2000 by Donald Becker and (1993) United States Government as represented by the
	Director, National Security Agency.
	This software may be used and distributed according to the terms of the GNU General Public License,
	incorporated herein by reference.

*/

#define ELKS

#include <arch/io.h>
#include <arch/irq.h>
#include <arch/ports.h>
#include <arch/segment.h>
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
#include <netinet/in.h>
#include "eth-msgs.h"

/* Offsets from base I/O address. */
#define EL3_DATA 0x00
#define EL3_CMD 0x0eU
#define EL3_STATUS 0x0eU
#define	EEPROM_READ 0x80U

#define EL3WINDOW(win_num) outw(SelectWindow + (win_num), ioaddr + EL3_CMD)


/* The top five bits written to EL3_CMD are a command, the lower
   11 bits are the parameter, if applicable. */
enum c509cmd {
	TotalReset = 0<<11, SelectWindow = 1<<11, StartCoax = 2<<11,
	RxDisable = 3<<11, RxEnable = 4<<11, RxReset = 5<<11, RxDiscard = 8<<11,
	TxEnable = 9<<11, TxDisable = 10<<11, TxReset = 11<<11,
	FakeIntr = 12<<11, AckIntr = 13<<11, SetIntrEnb = 14<<11,
	IntStatusEnb = 15<<11, SetRxFilter = 16<<11, SetRxThreshold = 17<<11,
	SetTxThreshold = 18<<11, SetTxStart = 19<<11, StatsEnable = 21<<11,
	StatsDisable = 22<<11, StopCoax = 23<<11, PowerUp = 27<<11,
	PowerDown = 28<<11, PowerAuto = 29<<11};

enum c509status {
	IntLatch = 0x0001U, AdapterFailure = 0x0002U, TxComplete = 0x0004U,
	TxAvailable = 0x0008U, RxComplete = 0x0010U, RxEarly = 0x0020U,
	IntReq = 0x0040U, StatsFull = 0x0080U, CmdBusy = 0x1000U, };

static int active_imask = IntLatch|RxComplete|TxAvailable|TxComplete|AdapterFailure|StatsFull;

/* The SetRxFilter command accepts the following classes: */
enum RxFilter {
	RxStation = 1, RxMulticast = 2, RxBroadcast = 4, RxProm = 8 };

/* Register window 1 offsets, the window used in normal operation. */
#define TX_FIFO		0x00U
#define RX_FIFO		0x00U
#define RX_STATUS 	0x08U
#define TX_STATUS 	0x0BU
#define TX_FREE		0x0CU		/* Remaining free bytes in Tx buffer. */

#define WN0_CONF_CTRL	0x04U		/* Window 0: Configuration control register */
#define WN0_ADDR_CONF	0x06U		/* Window 0: Address configuration register */
#define WN0_IRQ		0x08U		/* Window 0: Set IRQ line in bits 12-15. */
#define WN4_MEDIA	0x0AU		/* Window 4: Various transcvr/media bits. */
#define	MEDIA_TP	0x00C0U		/* Enable link beat and jabber for 10baseT. */
#define WN4_NETDIAG	0x06U		/* Window 4: Net diagnostic */
#define FD_ENABLE	0x8000U		/* Enable full-duplex ("external loopback") */
#define ENABLE_ADAPTER	0x1		/* Enable adapter (W0-conf-ctrl reg) */

#define EEPROM_NODE_ADDR_0	0x0	/* Word */
#define EEPROM_NODE_ADDR_1	0x1	/* Word */
#define EEPROM_NODE_ADDR_2	0x2	/* Word */
#define EEPROM_PROD_ID		0x3	/* 0x9[0-f]50 */
#define EEPROM_MFG_ID		0x7	/* 0x6d50 */
#define EEPROM_ADDR_CFG		0x8	/* Base addr */
#define EEPROM_RESOURCE_CFG	0x9	/* IRQ. Bits 12-15 */

#define EP_ID_PORT_START 0x110  /* avoid 0x100 to avoid conflict with SB16 */
#define EP_ID_PORT_INC 0x10
#define EP_ID_PORT_END 0x200
#define EP_TAG_MAX		0x7 /* must be 2^n - 1 */

static int INITPROC el3_isa_probe();
//static word_t read_eeprom(int, int);
static word_t id_read_eeprom(int);
static size_t el3_write(struct inode *, struct file *, char *, size_t);
static void el3_int(int, struct pt_regs *);
static size_t el3_read(struct inode *, struct file *, char *, size_t);
static void el3_release(struct inode *, struct file *);
static int el3_open(struct inode *, struct file *);
static int el3_ioctl(struct inode *, struct file *, unsigned int, unsigned int);
static int el3_select(struct inode *, struct file *, int);
static void el3_down();
static void update_stats();
void el3_sendpk(int, char *, int);
void el3_insw(int, char *, int);

extern void el3_mdelay(int);
extern struct eth eths[];

/* Maximum events (Rx packets, etc.) to handle at each interrupt. */
static int max_interrupt_work = 5;

/* runtime configuration set in /bootopts or defaults in ports.h */
#define net_irq     (netif_parms[ETH_EL3].irq)
#define net_port    (netif_parms[ETH_EL3].port)
#define net_ram     (netif_parms[ETH_EL3].ram)
#define net_flags   (netif_parms[ETH_EL3].flags)
static int ioaddr;	// FIXME  remove later
static word_t el3_id_port;
static unsigned char found;

static struct netif_stat netif_stat;
static char model_name[] = "3c509";
static char dev_name[] = "3c0";

static unsigned short verbose;
static unsigned char usecount;
static struct wait_queue rxwait;
static struct wait_queue txwait;

struct file_operations el3_fops =
{
    NULL,	 /* lseek */
    el3_read,
    el3_write,
    NULL,	 /* readdir */
    el3_select,
    el3_ioctl,
    el3_open,
    el3_release
};

void INITPROC el3_drv_init(void) {
	ioaddr = net_port;		// temporary

	verbose = (net_flags&ETHF_VERBOSE);
	if (el3_isa_probe() == 0) {
		found++;
		eths[ETH_EL3].stats = &netif_stat;
		/* The EL3 will grab and hold its default IRQ line
		 * unless we tell it not to. */
		EL3WINDOW(0);
		outw(0x0f00, ioaddr + WN0_IRQ);
	}

}

static int INITPROC el3_find_id_port ( void ) {

	for ( el3_id_port = EP_ID_PORT_START ;
	      el3_id_port < EP_ID_PORT_END ;
	      el3_id_port += EP_ID_PORT_INC ) {
		outb(0, el3_id_port);
		/* See if anything's listening */
		outb(0xff, el3_id_port);
		if (inb(el3_id_port) & 0x01) {
			/* Found a suitable port */
			//printk("el3: using ID port at %04x\n", el3_id_port);
			return 0;
		}
	}
	/* No id port available */
	printk("el3: no available ID port found\n");
	return -ENOENT;
}

/* Return 0 on success, 1 on error */
static int INITPROC el3_isa_probe( void )
{
	short lrs_state = 0xff;
	int i;
	word_t *mac = (word_t *)&netif_stat.mac_addr;
	byte_t *mac_addr = (byte_t *)mac;

	/* ISA boards are detected by sending the ID sequence to the
	   ID_PORT.  We find cards past the first by setting the 'current_tag'
	   on cards as they are found.  Cards with their tag set will not
	   respond to subsequent ID sequences.
	   ELKS supports only one card, comment kept for future reference. HS */

	if (el3_find_id_port()) return 1;

	outb(0x00, el3_id_port);
	outb(0x00, el3_id_port);	// just wait ...
	for (i = 0; i < 255; i++) {
		outb(lrs_state, el3_id_port);
		lrs_state <<= 1;
		lrs_state = lrs_state & 0x100 ? lrs_state ^ 0xcf : lrs_state;
	}
	outb(0xd0, el3_id_port);	// Set tag

	outb(0xd0, el3_id_port);		// select tag (0)
	outb(0xe0 |(ioaddr >> 4), el3_id_port );// Set IOBASE address, activate
	printk("eth: %s at %x, irq %d", dev_name, ioaddr, net_irq);

	if (id_read_eeprom(EEPROM_MFG_ID) != 0x6d50) {
		printk(" not found\n");
		return 1;
	}
	/* Read in EEPROM data.
	 * 3Com got the byte order backwards in the EEPROM. */
	for (i = 0; i < 3; i++)
		mac[i] = htons(id_read_eeprom(i));
	printk(", (%s) MAC %02x", model_name, (mac_addr[0]&0xff));
	i = 1;
	while (i < 6) printk(":%02x", (mac_addr[i++]&0xff));

	int iobase = id_read_eeprom(EEPROM_ADDR_CFG);
	//int if_port = iobase >> 14;
	if (verbose) printk(" (HWconf: %x/%d)", (0x200 + ((iobase & 0x1f) << 4)), (id_read_eeprom(9) >> 12));
	printk(", flags 0x%x\n", net_flags);
	
	return 0;
}

#if LATER
/* Read a word from the EEPROM using the regular EEPROM access register.
   Assume that we are in register window zero.
 */
static word_t read_eeprom(int addr, int index)
{
	outw(EEPROM_READ + index, addr + 10);
	/* Pause for at least 162 us. for the read to take place.
	   Some chips seem to require much longer */
	el3_mdelay(200);
	return inw(addr + 12);
}
#endif

/* Read a word from the EEPROM when in the ISA ID probe state. */
static word_t id_read_eeprom(int index)
{
	int bit, word = 0;

	/* Issue read command, and pause for at least 162 us. for it to complete.
	   Assume extra-fast 16Mhz bus. */
	outb(EEPROM_READ + index, el3_id_port);

	/* Pause for at least 162 us. for the read to take place. */
	/* Some chips seem to require much longer */
	el3_mdelay(400);

	for (bit = 15; bit >= 0; bit--)
		word = (word << 1) + (inb(el3_id_port) & 0x01);

	//printk("3c509 EEPROM word %d %04x.\n", index, word);

	return word;
}

static size_t el3_write(struct inode *inode, struct file *file, char *data, size_t len)
{
	int res;

	while (1) {

		prepare_to_wait_interruptible(&txwait);

		if (len > MAX_PACKET_ETH) len = MAX_PACKET_ETH;
		if (len < 64) len = 64;

		if (inw(ioaddr + TX_FREE) < (len + 4)) {
			// NO space in FIFO, wait
			if (file->f_flags & O_NONBLOCK) {
				res = -EAGAIN;
				break;
			}
			do_wait();
			if (current->signal) {
				res = -EINTR;
				break;
			}
		}
		//printk("T");
		//printk("T%d^", len);
		
		outw(SetIntrEnb | 0x0, ioaddr + EL3_CMD);	// Block interrupts
		
		el3_sendpk(ioaddr + TX_FIFO, data, len);

		/* Interrupt us when the FIFO has room for max-sized packet. */
		// FIXME: Unsure about this one ... Tune later
		outw(SetTxThreshold + 1536, ioaddr + EL3_CMD);

		outb(0x00, ioaddr + TX_STATUS); /* Pop the status stack. */
						/* may not be the right place to do this */

		/* Clear the Tx status stack. */
#if 0		// FIXME - check if this is OK to remove.
		{
			short tx_status;
			int i = 4;

			while (--i > 0	&& (tx_status = inb(ioaddr + TX_STATUS)) > 0) {
				//if (tx_status & 0x38) dev->stats.tx_aborted_errors++;
				if (tx_status & 0x30) outw(TxReset, ioaddr + EL3_CMD);
				if (tx_status & 0x3C) outw(TxEnable, ioaddr + EL3_CMD);
				outb(0x00, ioaddr + TX_STATUS); /* Pop the status stack. */
			}
		}
#endif
		// FIXME - may not be needed. Maybe trust interrupts ...
		//while (inb(ioaddr + EL3_STATUS) & CmdBusy) 
			//el3_mdelay(1);		// Wait - for now, to avoid collision

		res = len;
		outw(SetIntrEnb | active_imask, ioaddr + EL3_CMD);	// Reenable interrupts
		break;
	}
	return res;
}

/* *****************************************************************************************************
	A note about ELKS, EL3 and Interrupts
	ELKS does not service network interrupts as they arrive. Instead, when the application calls the
	driver, the status is checked and a read initiated if data is ready - or a write is initiated if 
	the interface is ready. Otherwise the application is but to sleep, to be awakened by the wake_up
	calls from the driver. A more traditional approach is to act on the cause of an interrupt 
	- e.g. transfer an arrived packet into a buffer, immediately.
	The 3Com 3C509 family of NICs expect the latter, and the RxComplete and RxEarly interrupt status
	bits can only be reset by emptying the NIC's FIFO.
	In order to get this scheme to work with ELKS, we mask off the RxComplete interrupt immediately 
	after seeing it, and reenable it in the packet read routine when the FIFO is empty. Not optimal
	from a performance point of view, but it works and will do for now. 

	As to Transmits, the TxComplete interrupt from the 3c5xx NICs indicate a transmit error. 
	The TxAvailable interrupt signals the availability of enough space in the output FIFO to hold a
	packet of a predetermined size. Since ELKS (ktcp) limits the send to 512 bytes and we rarely experiment 
	with sizes above 1k, this driver sets the limit to 1040 bytes.
	There are definite performance improvements to be gained by tuning this. 
 *******************************************************************************************************/

static void el3_int(int irq, struct pt_regs *regs)
{
	unsigned int status;
	int i = max_interrupt_work;

	outw(SetIntrEnb | 0x0, ioaddr + EL3_CMD);	// Block interrupts
	//printk("I");

	while ((status = inw(ioaddr + EL3_STATUS)) & active_imask) {
		//printk("/%04x;", status);

		if (status & (RxEarly | RxComplete)) {
			int err = inw(ioaddr + RX_STATUS);
			if (err & 0x4000) {	/* We have an error, record and recover */
				if (err & 0x3800) { 	/* packet error */
					if (verbose) printk(EMSG_RXERR, model_name, err);
					//printk("eth: RX error, status %04x len %d\n", err&0x3800, err&0x7ff);
					netif_stat.rx_errors++;
				} else {
					if (verbose) printk(EMSG_OFLOW, model_name, 0, netif_stat.oflow_keep);
					netif_stat.oflow_errors++;
				}
				outw(RxDiscard, ioaddr + EL3_CMD); /* Discard this packet. */
				inw(ioaddr + EL3_STATUS); 				/* Delay. */
				while ((err = inw(ioaddr + EL3_STATUS) & 0x1000)) {
					// FIXME: Doesn't seem to be a problem, delete printk later
				
					printk("eth: RX discard wait (%x)\n", err);
					el3_mdelay(1);
				}
			} else {
				wake_up(&rxwait);
				active_imask &= ~RxComplete;	// Disable RxComplete for now
			}

			//outw(SetRxThreshold | 60, ioaddr + EL3_CMD);	// Reactivate before ack
		}

		if (status & TxAvailable) {
			//printk("t:%02x;", inb(ioaddr + TX_STATUS));

			/* There's room in the FIFO for a full-sized packet. */
			outw(AckIntr | TxAvailable, ioaddr + EL3_CMD);
			wake_up(&txwait);
		}
		if (status & (AdapterFailure | StatsFull | TxComplete)) {
			/* Handle all uncommon interrupts. */
			if (status & StatsFull)			/* Empty statistics. */
				update_stats();

#if LATER		/* Candidate for optimization  */
			if (status & RxEarly) {
				//el3_rx(dev);
				outw(AckIntr | RxEarly, ioaddr + EL3_CMD);
				printk("RxEarly int\n");
			}
#endif
			if (status & TxComplete) {		/* Really Tx error. */
				short tx_status;
				int k = 4;

				if (verbose) printk(EMSG_TXERR, model_name, inb(ioaddr + TX_STATUS));
				netif_stat.tx_errors++;
				while (--k > 0 && (tx_status = inb(ioaddr + TX_STATUS)) > 0) {
					//if (tx_status & 0x38) dev->stats.tx_aborted_errors++;
					if (tx_status & 0x30) outw(TxReset, ioaddr + EL3_CMD);
					if (tx_status & 0x3C) outw(TxEnable, ioaddr + EL3_CMD);
					outb(0x00, ioaddr + TX_STATUS); /* Pop the status stack. */
				}
			}
			/* Adapter failure is either transmit overrun or receive underrun,
			 * both are really driver or host faults, should not happen. */
			if (status & AdapterFailure) {
				/* Adapter failure requires Rx reset and reinit. */
				if (verbose) printk(EMSG_ERROR, model_name, status);
				outw(RxReset, ioaddr + EL3_CMD);
				/* Set the Rx filter to the current state. */
				outw(SetRxFilter | RxStation | RxBroadcast, ioaddr + EL3_CMD);

				outw(RxEnable, ioaddr + EL3_CMD); /* Re-enable the receiver. */
				outw(AckIntr | AdapterFailure, ioaddr + EL3_CMD);
			}
		}

		if (--i < 0) {		/* Should not happen */
			printk("eth: Infinite loop in interrupt, status %4.4x.\n",
				   status);

			/* Clear all interrupts. */
			outw(AckIntr | 0xFF, ioaddr + EL3_CMD);
			break;
		}
		/* General acknowledge the IRQ. */
		outw(AckIntr | IntReq | IntLatch, ioaddr + EL3_CMD); /* Ack IRQ */
	}

	outw(SetIntrEnb | active_imask, ioaddr + EL3_CMD);
	//printk("i%04x!", inw(ioaddr + EL3_STATUS));

	return;
}


#if LATER
static struct net_device_stats *
el3_get_stats(struct net_device *dev)
{
	struct el3_private *lp = netdev_priv(dev);
	unsigned long flags;

	/*
	 *	This is fast enough not to bother with disable IRQ
	 *	stuff.
	 */

	spin_lock_irqsave(&lp->lock, flags);
	update_stats(dev);
	spin_unlock_irqrestore(&lp->lock, flags);
	return &dev->stats;
}

#endif

/*  Update statistics.  We change to register window 6, so this should be run
	single-threaded if the device is active. This is expected to be a rare
	operation, and it's simpler for the rest of the driver to assume that
	window 1 is always valid rather than use a special window-state variable.
	*/
/* ELKS: Dummy for now */
static void update_stats( void )
{

	//printk("eth: Updating statistics.\n");

	/* Turn off statistics updates while reading. */
	outw(StatsDisable, ioaddr + EL3_CMD);

	/* Switch to the stats window, and read everything. */
	EL3WINDOW(6);
#if 0
	dev->stats.tx_carrier_errors 	+= inb(ioaddr + 0);
	dev->stats.tx_heartbeat_errors	+= inb(ioaddr + 1);
	/* Multiple collisions. */	   inb(ioaddr + 2);
	dev->stats.collisions		+= inb(ioaddr + 3);
	dev->stats.tx_window_errors	+= inb(ioaddr + 4);
	dev->stats.rx_fifo_errors	+= inb(ioaddr + 5);
	dev->stats.tx_packets		+= inb(ioaddr + 6);
	/* Rx packets */		   inb(ioaddr + 7);
	/* Tx deferrals */		   inb(ioaddr + 8);
#else
	{
	int i = 0;
	while (i++ < 9) inb(ioaddr + i);
	}
#endif
	// The final two are word size
	inw(ioaddr + 10);	/* Total Rx and Tx octets. */
	inw(ioaddr + 12);

	/* Back to window 1, and turn statistics back on. */
	EL3WINDOW(1);
	outw(StatsEnable, ioaddr + EL3_CMD);
}


/*
 * Release (close) device
 */

static void el3_release(struct inode *inode, struct file *file)
{
	if (--usecount == 0) {
		el3_down();

		/* Switching to window 0 disables the IRQ. */
		EL3WINDOW(0);

		/* But we explicitly zero the IRQ line select anyway */
		outw(0x0f00, ioaddr + WN0_IRQ);

		free_irq(net_irq);
	}
}


static size_t el3_read(struct inode *inode, struct file *filp, char *data, size_t len)
{
	short rx_status;
	size_t res;

	while(1) {
		
		rx_status = inw(ioaddr + RX_STATUS);
		//printk("R%04x", rx_status);		// DEBUG
		prepare_to_wait_interruptible(&rxwait);

		if (rx_status & 0x8000) {	// FIFO empty or incomplete
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

		outw(SetIntrEnb | 0x0, ioaddr + EL3_CMD);	// Block interrupts
		if (rx_status & 0x4000) {	/* Error, should not happen	*/
						/* Should be caught in the int handler */
			short error = rx_status & 0x3800;

			outw(RxDiscard, ioaddr + EL3_CMD);
			printk("3c509: Error in read (%04x), buffer cleared\n", error);
			inw(ioaddr + EL3_STATUS); 				/* Delay. */
			while ((res = inw(ioaddr + EL3_STATUS) & 0x1000)) {
				/// FIXME: printk to be removed	later, seems stable
				printk("eth: RD discard delay (%x)\n", res);
				el3_mdelay(1);
			}
			res = -EIO;
		} else {
			short pkt_len = rx_status & 0x7ff;
			el3_insw(ioaddr + RX_FIFO, data, (pkt_len + 1) >> 1); //Word size

			outw(RxDiscard, ioaddr + EL3_CMD); /* Pop top Rx packet. */
			res = pkt_len;
			
		}
		break;
	}
	
	finish_wait(&rxwait);
	active_imask |= RxComplete;	/* Reactivate recv interrupts */

	outw(SetIntrEnb | active_imask, ioaddr + EL3_CMD);
	//printk("r%x:", inw(ioaddr + RX_STATUS));
	return res;
}

static void el3_down( void )
{

	/* Turn off statistics ASAP.  We update lp->stats below. */
	outw(StatsDisable, ioaddr + EL3_CMD);

	/* Disable the receiver and transmitter. */
	outw(RxDisable, ioaddr + EL3_CMD);
	outw(TxDisable, ioaddr + EL3_CMD);

#if 0	/* Some times this prevents TP from coming back on line in the next open() */

	/* Disable link beat and jabber */
	if (!(net_flags&ETHF_USE_AUI)) {
		EL3WINDOW(4);
		outw(inw(ioaddr + WN4_MEDIA) & ~MEDIA_TP, ioaddr + WN4_MEDIA);
		el3_mdelay(500);
	}
#endif

	EL3WINDOW(1);
	outw(SetIntrEnb | 0x0, ioaddr + EL3_CMD);
}

/* Open el3 device: All initialization is done here, dev_init does the probe 
 * (verify existence) and pulls out the MAC address, that's it.
 */

static int el3_open(struct inode *inode, struct file *file)
{
	int i, err;
	char *mac_addr = (char *)&netif_stat.mac_addr;

	if (!found) 
		return -ENODEV;

	if (usecount++ != 0)
		return 0;		// Already open, success

	err = request_irq(net_irq, el3_int, INT_GENERIC);
	if (err) {
		printk(EMSG_IRQERR, model_name, net_irq, err);
		return err;
	}
	EL3WINDOW(0);
	/* Activating the board - done in _init, repeat doesn't harm */
	outw(ENABLE_ADAPTER, ioaddr + WN0_CONF_CTRL);

	/* Set the IRQ line. */
	outw((net_irq << 12), ioaddr + WN0_IRQ);

	//printk("el3: IOBASE %x\n", inw(ioaddr + WN0_ADDR_CONF) & 0xf);

	/* Set the station address in window 2 each time opened. */
	EL3WINDOW(2);
	for (i = 0; i < 6; i++)
		outb(mac_addr[i], ioaddr + i);

	EL3WINDOW(1);
	outw(RxReset, ioaddr + EL3_CMD);
	outw(TxReset, ioaddr + EL3_CMD);

	for (i = 0; i < 31; i++)
		inb(ioaddr+TX_STATUS);		/* Clear TX status stack */

	outw(AckIntr | 0xff, ioaddr + EL3_CMD);	/* Get rid of stray interrupts */

	/* The IntStatusEnb reg defines which interrupts will be visible,
	 * The SetIntrEnb reg defines which of the visible interrupts will actually 
	 * trigger an interrupt (the INTR mask). */
	outw(SetIntrEnb | active_imask, ioaddr + EL3_CMD);

	/* Unless the flags say USE_AUI, activate link beat and jabber check for TP */
	if (!(net_flags & ETHF_USE_AUI)) {
		EL3WINDOW(4);
		outw(inw(ioaddr + WN4_MEDIA) | MEDIA_TP, ioaddr + WN4_MEDIA);
		el3_mdelay(1000);
	}

	EL3WINDOW(1);
	
	/* FIXME - decrease this value to see if it impacts performance */
	outw(SetTxThreshold | 1536, ioaddr + EL3_CMD); /* Signal TxAvailable when this # of 
							* bytes is available */
	//outw(SetRxThreshold | 60, ioaddr + EL3_CMD);	// Receive interrupts when 60 bytes
							// are available

#if LATER	// This is tuning, fix later.
		// NOTE: Enabling full duplex is probably a bad idea, see
		// https://www.kernel.org/doc/html/v5.17/networking/device_drivers/ethernet/3com/3c509.html
	{
		int sw_info, net_diag;

		/* Combine secondary sw_info word (the adapter level) and primary
			sw_info word (duplex setting plus other useless bits) */
		EL3WINDOW(0);
		sw_info = (read_eeprom(ioaddr, 0x14) & 0x400f) |
			(read_eeprom(ioaddr, 0x0d) & 0xBff0);
		//printk("sw_info %04x ", sw_info);
		EL3WINDOW(4);
		//net_diag = inw(ioaddr + WN4_NETDIAG);
		//net_diag = (net_diag | FD_ENABLE); /* temporarily assume full-duplex will be set */
		//pr_info("%s: ", dev->name);

		switch (dev->if_port & 0x0c) {
			case 12:
				/* force full-duplex mode if 3c5x9b */
				if (sw_info & 0x000f) {
					pr_cont("Forcing 3c5x9b full-duplex mode");
					break;
				}
				fallthrough;
			case 8:
				/* set full-duplex mode based on eeprom config setting */
				if ((sw_info & 0x000f) && (sw_info & 0x8000)) {
					pr_cont("Setting 3c5x9b full-duplex mode (from EEPROM configuration bit)");
					break;
				}
				fallthrough;
			default:
				/* xcvr=(0 || 4) OR user has an old 3c5x9 non "B" model */
				pr_cont("Setting 3c5x9/3c5x9B half-duplex mode");
				net_diag = (net_diag & ~FD_ENABLE); /* disable full duplex */
		}

		//outw(net_diag, ioaddr + WN4_NETDIAG);
		//printk("%s: 3c5x9 net diag word is now: %4.4x.\n", dev->name, net_diag);
		/* Enable link beat and jabber check. */
		outw(inw(ioaddr + WN4_MEDIA) | MEDIA_TP, ioaddr + WN4_MEDIA);
	}
#endif

	/* Switch to the stats window, and clear all stats by reading. */
	outw(StatsDisable, ioaddr + EL3_CMD);
	EL3WINDOW(6);
	for (i = 0; i < 9; i++)
		inb(ioaddr + i);
	inw(ioaddr + 10);
	inw(ioaddr + 12);

	/* Switch to register set 1 for normal use. */
	EL3WINDOW(1);

	/* Accept b-cast and phys addr only. */
	outw(SetRxFilter | RxStation | RxBroadcast, ioaddr + EL3_CMD);
	//outw(StatsEnable, ioaddr + EL3_CMD); /* Turn on statistics. */

	outw(RxEnable, ioaddr + EL3_CMD); /* Enable the receiver. */
	outw(TxEnable, ioaddr + EL3_CMD); /* Enable transmitter. */

	/* Allow status bits to be seen. */
	outw(IntStatusEnb | 0xff, ioaddr + EL3_CMD);

	outw(SetIntrEnb | active_imask, ioaddr + EL3_CMD);
	return 0;
}

/*
 * I/O control
 */

static int el3_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned int arg)
{       
	int err = 0;
	byte_t *mac_addr = (byte_t *)&netif_stat.mac_addr;
	
	switch (cmd) {
		case IOCTL_ETH_ADDR_GET:
			err = verified_memcpy_tofs((byte_t *)arg, mac_addr, 6);
			break;

		case IOCTL_ETH_GETSTAT:
			/* Return the entire netif_struct */
			err = verified_memcpy_tofs((char *)arg, &netif_stat, sizeof(netif_stat));
			break;
		
		default:
			err = -EINVAL;
	
	}
	return err;
}

/*
 * Test for readiness
 */

int el3_select(struct inode *inode, struct file *filp, int sel_type)
{       
	int res = 0;
	
	/* Need to block interrupts to avoid a race condition which happens if
	   an interrupt happens just after we check RX_STATUS. The interrupt
	   will turn off recdeive interrupts and select doesn't know that we have data.
	 */
	outw(SetIntrEnb | 0x0, ioaddr + EL3_CMD);
	switch (sel_type) {
		case SEL_OUT:
			// FIXME - may need a more accurate tx status test
			if (inw(ioaddr + TX_FREE) < MAX_PACKET_ETH) {
				select_wait(&txwait);
				break;
			}
			res = 1;
			break;
		
		case SEL_IN:

			// Don't use RxComplete for this test, it has been masked out!
			if (inw(ioaddr+RX_STATUS) & 0x8000) {
				//printk("s%x", inw(ioaddr+RX_STATUS));
				select_wait(&rxwait);
				break;
			}
			res = 1;
			break;
		
		default:
			res = -EINVAL;
	}
	//printk("S%d", res);
	outw(SetIntrEnb | active_imask, ioaddr + EL3_CMD);	// reenable interrupts
	return res;
}

