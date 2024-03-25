/*
 *
 * NE2K Ethernet driver - supports NICs using the NS DP8390 and
 * compatible chip sets. Tested with Eagle 8390 (6321839050001 PnP) and
 * Winbond W89C902P based cards.
 *
 * 8 bit i/f support and autoconfig added by Helge Skrivervik (@mellvik) april 2022
 * based on the RTL8019AS chip in the interface card from Weird Electronics.
 *
 */

#include <linuxmt/errno.h>
#include <linuxmt/major.h>
#include <linuxmt/ioctl.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/fs.h>
#include <linuxmt/sched.h>
#include <linuxmt/limits.h>
#include <linuxmt/mm.h>
#include <linuxmt/string.h>
#include <linuxmt/debug.h>
#include <linuxmt/netstat.h>
#include <arch/io.h>
#include <arch/irq.h>
#include "eth-msgs.h"

// Shared declarations between low and high parts

#include "ne2k.h"

#define CHK8390_TINY     1  /* perform quick 8390 chip detect in NIC probe */
#define CHK8390_FULL     0  /* perform more robust 8390 chip detect in NIC probe */

/* runtime configuration set in /bootopts or defaults in ports.h */
#define net_irq     (netif_parms[ETH_NE2K].irq)
#define NET_PORT    (netif_parms[ETH_NE2K].port)
#define net_flags   (netif_parms[ETH_NE2K].flags)
int net_port;	    /* required for ne2k-asm.S */


static unsigned char usecount;
static unsigned char found;
static unsigned int verbose;
static struct wait_queue rxwait;
static struct wait_queue txwait;
static struct netif_stat netif_stat;
static byte_t model_name[] = "ne2k";
static byte_t dev_name[] = "ne0";

extern int ne2k_next_pk;
extern word_t ne2k_flags;
extern word_t ne2k_has_data;
extern struct eth eths[];

/*
 * Read a complete packet from the NIC buffer
 */

static size_t ne2k_read(struct inode *inode, struct file *filp, char *data, size_t len)
{
	size_t res;
	word_t nhdr[2];	/* buffer header from the NIC, for debugging */

	while (1) {
		size_t size;  // actual packet size

		//printk("R");
		prepare_to_wait_interruptible(&rxwait);
		if (!ne2k_has_data) {
		//if (ne2k_rx_stat() != NE2K_STAT_RX) {

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
		size = ne2k_pack_get(data, len, nhdr);

		//printk("r%04x|%04x/",nhdr[0], nhdr[1]);	// NIC buffer header
		debug_eth("ne0: read: req %d, got %d real %d\n", len, size, nhdr[1]);

		//if ((nhdr[1] > size) || (nhdr[0] == 0)) {
		if ((nhdr[0]&~0x7f21) || (nhdr[0] == 0)) {	//EXPERIMENTAL: Upper byte = block #, max 7f

			/* Sanity check, should not happen.
			 * If this happens, we're reading garbage from the NIC, all pointers
			 * may be invalid, clear device and buffers.
			 *	
			 * Likely reason: We have a 8 bit interface running with 16k buffer enabled.
			 * 
			 * May want to add more tests for nhdr[0]:
			 * 	Low byte should be 1 or 21	(receive status reg)
			 *	High byte is a pointer to the next packet in the NIC ring buffer,
			 *		should be < 0x80 and > 0x45
			 */

			netif_stat.rq_errors++;
			printk("$%04x.%02x$", ne2k_getpage(), ne2k_next_pk&0xff);
			if (verbose) printk(EMSG_DMGPKT, dev_name, nhdr[0], nhdr[1]);

#if 0
			if (nhdr[0] == 0) { 	// When this happens, the NIC has serious trouble,
						// need to reset as if we had a buffer overflow.
				res = ne2k_clr_oflow(0); 
				//printk("<%04x>", res);
			} else
#endif
				ne2k_rx_init();	// Resets the ring buffer pointers to initial values,
						// effectively purging the buffer.
			res = -EIO;
			break;
		}
		res = size;
		break;
	}

	finish_wait(&rxwait);
	return res;
}

/*
 * Pass packet to driver for send
 */

static size_t ne2k_write(struct inode *inode, struct file *file, char *data, size_t len)
{
	size_t res;

	while (1) {
		prepare_to_wait_interruptible(&txwait);

		// tx_stat() checks the command reg, not the tx_status_reg!
		if (ne2k_tx_stat() != NE2K_STAT_TX) {

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

		if (len > MAX_PACKET_ETH) len = MAX_PACKET_ETH;

		if (len < 64) len = 64;  /* issue #133 */
		ne2k_pack_put(data, len);

		res = len;
		break;
	}

	finish_wait(&txwait);
	return res;
}

/*
 * Test for readiness
 */

int ne2k_select(struct inode *inode, struct file *filp, int sel_type)
{
	int res = 0;

	switch (sel_type) {
		case SEL_OUT:
			if (ne2k_tx_stat() != NE2K_STAT_TX) {
				select_wait(&txwait);
				break;
			}
			res = 1;
			break;

		case SEL_IN:
			//if (ne2k_rx_stat() != NE2K_STAT_RX) {
			if (!ne2k_has_data) {
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
 * Interrupt handler
 */

static void ne2k_int(int irq, struct pt_regs *regs)
{
	word_t stat, page;

	//printk("/");
	while (1) {
		stat = ne2k_int_stat();
		if (!stat) break; 	/* If zero, we're done! */
#if 0	/* debug */
		page = ne2k_getpage();
		printk("$%04x.%02x$", page,ne2k_next_pk&0xff);
#endif
        	if (stat & NE2K_STAT_OF) {
			netif_stat.oflow_errors++;
			if (verbose) printk(EMSG_OFLOW, dev_name, stat, netif_stat.oflow_keep);
			page = ne2k_clr_oflow(netif_stat.oflow_keep); 

			debug_eth("/CB%04x/ ", page);
			//printk("%04x/ ", page);

			if (ne2k_has_data)
				wake_up(&rxwait);
			break; 
		}

		if (stat & NE2K_STAT_RX) {
			ne2k_has_data = 1; 	// data available
			wake_up(&rxwait);
			outb(NE2K_STAT_RX, net_port + EN0_ISR); // Clear intr bit
		}

		if (stat & NE2K_STAT_TX) {
			outb(NE2K_STAT_TX, net_port + EN0_ISR); // Clear intr bit
			inb(net_port + EN0_TSR);
			//ne2k_get_tx_stat();	// clear the TX bit in the ISR 
			wake_up(&txwait);
		}
		debug_eth("%02X/%d/", stat, ne2k_has_data);
		/* shortcut for speed */
		if (!(stat&~(NE2K_STAT_TX|NE2K_STAT_RX|NE2K_STAT_OF))) continue;

		/* These don't happen very often */
		if (stat & NE2K_STAT_RDC) {
			//printk("ne0: Warning - RDC intr. (0x%02x)\n", stat);
			/* RDC interrupts should be masked in the low level driver.
		 	 * The dma_read, dma_write routines will fail if the RDC intr
			 * bit is cleared elsewhere.
		 	 * OTOH: The RDC bit will occasionally be set for other reasons 
			 * (such as an aborted remote DMA transfer). In such cases the
			 * RDC bit must be reset here, otherwise this function will loop.
		 	 */
			ne2k_rdc();
		}

		if (stat & NE2K_STAT_TXE) { 	
			int k;
			/* transmit error detected, this should not happen. */
			/* ne2k_get_tx_stat resets this bit in the ISR */
			netif_stat.tx_errors++;
			k = ne2k_get_tx_stat();	// read tx status reg to keep the NIC happy
			if (verbose) printk(EMSG_TXERR, dev_name, k);
		}

		/* RXErrors occur almost exclusively when using an 8 bit interface.
		 * A bug in QEMU will cause continuous interrupts if RXE intr is unmasked.
		 * Therefore the low level driver will unmask RXE intr only if the NIC is 
		 * running in 8 bit mode */
		if (stat & NE2K_STAT_RXE) { 	
			/* Receive error detected, may happen when traffic is heavy */
			/* The 8 bit interface gets lots of these, all CRC, packet dropped */
			/* Don't do anything, just count, report & clr */
			netif_stat.rx_errors++;
			if (verbose) printk(EMSG_RXERR, dev_name, inb(net_port + EN0_RSR));
			outb(NE2K_STAT_RXE, net_port + EN0_ISR); // Clear intr bit
		}
		if (stat & NE2K_STAT_CNT) { 	
			/* The tally counters will overflow on 8 bit interfaces with
		 	 * lots of overruns.  Just clear the condition */
			ne2k_clr_err_cnt();
		}
	}
	//printk(".%02x", ne2k_int_stat());
	stat = page;		/* to keep compiler happy */
}

/*
 * I/O control
 */

static int ne2k_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned int arg)
{
	int err = 0;
	byte_t *mac_addr = (byte_t *)&netif_stat.mac_addr;

	switch (cmd) {
		case IOCTL_ETH_ADDR_GET:
			err = verified_memcpy_tofs((byte_t *)arg, mac_addr, 6);
			break;

#if LATER
		case IOCTL_ETH_ADDR_SET:
			if ((err = verified_memcpy_fromfs(mac_addr, (byte_t *) arg, 6)) != 0)
				break;
			ne2k_addr_set(mac_addr);
			printk("ne0: MAC address changed to %02x", mac_addr[0]);
			for (i = 1; i < 6; i++) printk(":%02x", mac_addr[i]);
			printk("\n");
			break;
#endif

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
 * Device open
 */

static int ne2k_open(struct inode *inode, struct file *file)
{
	if (!found)
		return -ENODEV;

	if (usecount++ == 0) {	// Don't initialize if already open
		int err = request_irq(net_irq, ne2k_int, INT_GENERIC);
		if (err) {
			printk(EMSG_IRQERR, dev_name, net_irq, err);
			return err;
		}
		ne2k_reset();
		ne2k_init();
		ne2k_start();
	}
	return 0;
}

/*
 * Release (close) device
 */

static void ne2k_release(struct inode *inode, struct file *file)
{
	if (--usecount == 0) {
		ne2k_stop();
		free_irq(net_irq);
	}
}

/*
 * Ethernet operations
 */

struct file_operations ne2k_fops =
{
    NULL,         /* lseek */
    ne2k_read,
    ne2k_write,
    NULL,         /* readdir */
    ne2k_select,
    ne2k_ioctl,
    ne2k_open,
    ne2k_release
};

#if DEBUG_ETH
void ne2k_display_status(void)
{
	printk("\n---- Ethernet Stats ----\n");
	printk("Skip Count %d\n", netif_stat.oflow_keep);
	printk("Receive overflows %d\n", netif_stat.oflow_errors);
	printk("Receive errors %d\n", netif_stat.rx_errors);
	printk("NIC buffer errors %d\n", netif_stat.rq_errors);
	printk("Transmit errors %d\n", netif_stat.tx_errors);
}
#endif

/* return 0 if NE2K NIC present */
static word_t INITPROC ne2k_probe(void)
{
    int reg0;

#define ENODMA_PAGE0        0x20
#define ENODMA_PAGE0_STOP   0x21
#define ENODMA_PAGE1_STOP   0x61

    /* send 0x21 to NIC and read result */
    outb(ENODMA_PAGE0_STOP, net_port);
    reg0 = inb(net_port);

#if CHK8390_TINY
    /* quick 8390 chip detect, from E2100 8390 check by Donald Becker */
    if (reg0 != 0x21 && reg0 != 0x23)
        return 1;   /* NIC not present */
#else
    /* original probe */
    if (reg0 == 0xFF || reg0 == 0)
        return 1;   /* NIC not present */
#endif

#if CHK8390_FULL
    /* more robust 8390 chip detect, from Linux 2.0 NE2K driver */
    outb(ENODMA_PAGE1_STOP, net_port);  /* send 0x61 */
    outb(0xff, net_port + EN0_TXCR);
    outb(ENODMA_PAGE0, net_port);
    inb(net_port + EN0_CNTR0);          /* clear the counter by reading */
    if (inb(net_port + EN0_CNTR0) != 0)
        return 1;   /* not 8390 */
#endif

    return 0;       /* NIC present */
}

/*
 * Ethernet main initialization (during boot)
 * FIXME: Needs return value to signal that initalization failed.
 */

void INITPROC ne2k_drv_init(void)
{
	int err, i;
	word_t prom[16];	/* PROM containing HW MAC address and more 
				 * (aka SAPROM, Station Address PROM).
				 * PROM size is 16 bytes. If read in word (16 bit) mode,
				 * the upper byte is either a copy of the lower or 00.
				 * depending on the chip. This may be used to detect 8 vs 16
				 * bit cards automagically.
				 * The address occupies the first 6 bytes, followed by a 'card signature'.
				 * If bytes 14 & 15 == 0x57, this is a ne2k clone.
				 */
	byte_t *cprom, *mac_addr;

	netif_stat.oflow_keep = 0;	/* Default - clear buffer if overflow.
					 * Testing indicates 0 means faster recovery under heavy
					 * load if buffer < 16k */
	mac_addr = (byte_t *)&netif_stat.mac_addr;

	net_port = NET_PORT;    // ne2k-asm.S needs this.

	while (1) {
		int j, k;

		err = ne2k_probe();
		verbose = (net_flags&ETHF_VERBOSE);
		printk("eth: %s at %x, irq %d", dev_name, net_port, net_irq);
		if (err) {
			printk(" not found\n");
			break;
		}

#if DELETEME
		/* Should not happen since we already have probed the NIC successfully */
		/* May be superflous - candidate for removal */
		if (((prom[0]&0xff) == 0xff) && ((prom[1]&0xff) == 0xff)) {
			printk("%s: No MAC address\n", dev_name);
			err = -1;
			break;
		} 
#endif
		found = 1;
		cprom = (byte_t *)prom;
		ne2k_get_hw_addr(prom);

		err = j = k = 0;
		//for (i = 0; i < 32; i++) printk("%02x", cprom[i]);
		//printk("\n");

		/* if the high byte of every word is 0, this is a 16 bit card
		 * if the high byte = low byte in every word, this is probably QEMU */
		for (i = 1; i < 12; i += 2) j += cprom[i];
		for (i = 0; i < 12; i += 2) k += cprom[i];	/* QEMU check */

		/* ne2k_flags may be used as a simple variable until
		 * we add in the buffer flags below */
		if (j && (j!=k)) {	
			ne2k_flags = ETHF_8BIT_BUS;
			model_name[2] = '1';
			netif_stat.if_status |= NETIF_AUTO_8BIT; 
		} else {
			for (i = 0; i < 16; i++) cprom[i] = (char)prom[i]&0xff;
			ne2k_flags = 0;
		}
		if (j == k && !memcmp(cprom, mac_addr, 5)) netif_stat.if_status |= NETIF_IS_QEMU;
		//for (i = 0; i < 16; i++) printk("%02x", cprom[i]);
		//printk("\n");

		memcpy(mac_addr, cprom, 6);
		printk(", (%s) MAC %02x", model_name, mac_addr[0]);
		ne2k_addr_set(cprom);   /* Set NIC mac addr now so IOCTL works */

		i = 1;
		while (i < 6) printk(":%02x", mac_addr[i++]);
		if (net_flags&ETHF_8BIT_BUS) {
			/* flag that we're forcing 8 bit bus on 16 NIC */
			if (!ne2k_flags) printk(" (8bit)");
			ne2k_flags = ETHF_8BIT_BUS; 	/* Forced 8bit */
		}
		if (!(ne2k_flags&ETHF_8BIT_BUS))
			netif_stat.oflow_keep = 3;	// Experimental: use 3 if 16k buffer
		if (netif_stat.if_status & NETIF_IS_QEMU) 
			printk(" (QEMU)");

		/* The _BUF flags indicate forced buffer size, ZERO means use defaults */
		if (net_flags&(ETHF_4K_BUF|ETHF_8K_BUF|ETHF_16K_BUF)) {
			ne2k_flags |= net_flags&0xf;		/* asm code uses this */
			printk(" (%dk buffer)", 4<<(net_flags&0x3));
		}
		printk(", flags 0x%02x\n", net_flags);

#if DEBUG_ETH
		debug_setcallback(2, ne2k_display_status);
#endif
		break;
	}
	ne2k_has_data = 0;
	eths[ETH_NE2K].stats = &netif_stat;
}
