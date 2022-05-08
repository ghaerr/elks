/*
 *
 * NE2K Ethernet driver - supports NICs using the NS DP8390 and
 * compatible chip sets. Tested with Eagle 8390 (6321839050001 (PnP) and
 * Winbond W89C902P based cards.
 *
 * 8bit i/f support and autoconfig added by Helge Skrivervik (@mellvik) april 2022
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
#include <linuxmt/debug.h>

// Shared declarations between low and high parts

#include "ne2k.h"

int net_irq = NE2K_IRQ;	/* default IRQ, changed by netirq= in /bootopts */
int net_port = NE2K_PORT; /* default IO PORT, changed by netport= in /bootopts */
word_t net_rx_errs;	/* count receive errors */
word_t net_tx_errs;	/* transmit errors */
word_t net_oflow_errs; /* receive buffer overflow counter */

// Static data
struct wait_queue rxwait;
struct wait_queue txwait;

static byte_t ne2k_inuse = 0;

static byte_t mac_addr[6]  = {0x52, 0x54, 0x00, 0x12, 0x34, 0x57};  /* QEMU default */
			     /* Overwritten by actual HW MAC address if found */
static word_t _ne2k_skip_cnt;

extern word_t _ne2k_next_pk;
extern word_t _ne2k_is_8bit;
extern word_t _ne2k_has_data;

/*
 * Read a complete packet from the NIC buffer
 */

static size_t ne2k_read(struct inode *inode, struct file *filp, char *data, size_t len)
{
	size_t res;
	word_t nhdr[2];	/* packet header from the NIC, for debugging */

	while (1) {
		size_t size;  // actual packet size

		//printk("R");
		//printk("R%02x;", ne2k_int_stat()&0xff);
		prepare_to_wait_interruptible(&rxwait);
		if (ne2k_rx_stat() != NE2K_STAT_RX) {

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
		if ((size = ne2k_pack_get(data, len, nhdr)) < 0) {
			res = -EIO;
			break;
		}

		//printk("r%04x|%04x/",nhdr[0], nhdr[1]);
		debug_eth("eth read: req %d, got %d real %d\n", len, size, nhdr[1]);
		if ((nhdr[1] > size) || (nhdr[0] == 0)) {
			/* This is a sanity check, should not happen.
			 * 1) The buffer size is too small (should be 1536)
			 * 2) Driver problem and nhdr[] is not the actual NIC header
			 * 3) Hardware problem since the NIC is programmed to not accept large packets from the wire
			 *	
			 * Most likely: We have a 8bit interface running with 16k buffer enabled.
			 * In that case, well end up here all the time and eventually hang.
			 */

			printk("eth: truncated read (%04x %d), clearing buffer\n", nhdr[0], nhdr[1]);
			if (nhdr[0] == 0) { 	// must clear and reset
				res = ne2k_clr_oflow(0); 
				//printk("<%04x>", res);
			} else
				ne2k_rx_init();	// Resets the ring buffer pointers to initial values,
						// effectively purging the buffer.
			_ne2k_has_data = 0;
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
		//printk("T%d,", len);
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
		if ((res = ne2k_pack_put(data, len))) {	///FIXME
			//printk(">%d<", res);	// DEBUG STUFF
			if (res == 0xff) {
				res = -EIO;
				break;
			}
		}

		res = len;
		break;
	}

	finish_wait(&txwait);
	//printk("t");
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
			if (ne2k_rx_stat() != NE2K_STAT_RX) {
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

	while (1) {	
		stat = ne2k_int_stat();
		if (!(stat & ~NE2K_STAT_RDC)) break;	/* In QEMU, the RDC bit is always set 
							 * when RXE is enabled */
		//printk("/%x", stat);
#if 0
		page = ne2k_getpage();
		printk("$%04x.%02x$", page,_ne2k_next_pk&0xff);
#endif

        	if (stat & NE2K_STAT_OF) {
			printk("eth: receive oflow (0x%x), keep %d\n", stat, _ne2k_skip_cnt);
			page = ne2k_clr_oflow(_ne2k_skip_cnt); 	// 1+ -> keep this # of pkts
								// 0 -> dump all
			debug_eth("/CB%04x/ ", page);
			//printk("*CB%04x", page);

			net_oflow_errs++;

			if (_ne2k_has_data)
				wake_up(&rxwait);

			continue;
		}

		if (stat & NE2K_STAT_RX) {
			ne2k_get_rx_stat();	// Clear INTR reg bit
			_ne2k_has_data = 1; 	// data available
			wake_up(&rxwait);
		}

		if (stat & NE2K_STAT_TX) {
			ne2k_get_tx_stat();	// just read the tx status reg to keep the NIC
						// happy - reset the INTR reg bit
			wake_up(&txwait);
		}
#if 0	/* QEMU confuses RXE and RDC interrupts - have to turn this off
      	 * to run in QEMU */
		if (stat & NE2K_STAT_RDC) {
			printk("eth: Warning - RDC intr. (0x%02x)\n", stat);
			/* The RDC interrupt should be disabled in the low level driver.
		 	* When real DMA transfer from NIC to system RAM is enabled, this is where
		 	* we handle transfer completion.
		 	* NOTICE: If we get here, a remote DMA transfer was aborted. This should
		 	* not happen.
		 	*/
			ne2k_rdc();
		}
#endif
		if (stat & NE2K_STAT_TXE) { 	
			/* transmit error detected, this should not happen. */
			/* ne2k_get_tx_stat resets this int in the ISR */
			net_tx_errs++;
			printk("eth: TX-error, status 0x%02x\n", ne2k_get_tx_stat());
			ne2k_get_tx_stat();	// just read the tx status reg to keep the NIC
		}
		if (stat & NE2K_STAT_RXE) { 	
			/* Receive error detected, may happen when traffic is heavy */
			/* The 8bit interface gets lots of these */
			/* ne2k_get_rx_stat resets this int in the ISR */
			net_rx_errs++;
			ne2k_clr_rxe();
			printk("eth: RX-error, status 0x%02x\n", ne2k_get_rx_stat());
		}
		if (stat & NE2K_STAT_CNT) { 	
			/* The tally counters will overflow on 8 bit interfaces with
		 	* lots of overruns.  Just clear the condition */
			ne2k_clr_err_cnt();
		}
		debug_eth("%02X/%d/", stat, _ne2k_has_data);
	}
	//printk(".");
	stat = page;		/* to keep compiler happy */
}

/*
 * I/O control
 */

static int ne2k_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned int arg)
{
	int err = 0;

	switch (cmd) {
		case IOCTL_ETH_ADDR_GET:
			memcpy_tofs((char *) arg, mac_addr, 6);
			break;

#if 0 /* unused*/
		case IOCTL_ETH_ADDR_SET:
			memcpy_fromfs(mac_addr, (char *) arg, 6);
			ne2k_addr_set(mac_addr);
			break;

		case IOCTL_ETH_HWADDR_GET:
			/* Get the hardware address of the NIC,	which may be different
			 * from the currently programmed address. Be careful with this,
			 * it may interrupt ongoing send/receives.
			 * arg must be byte[32].
			 */
			ne2k_get_hw_addr((word_t *) arg);
			break;

		case IOCTL_ETH_GETSTAT:
			/* Get statistics from the NIC hardware (error counts etc.)
			 * arg is byte[3].
			 */
			ne2k_get_errstat((byte_t *)arg);
			break;

		case IOCTL_ETH_OFWSKIP_SET:
			/* Set the number of packets to skip @ ring buffer overflow. */
			_ne2k_skip_cnt = arg;
			debug_eth("eth: OFW skip-cnt is now %d.\n", _ne2k_skip_cnt);
			break;

		case IOCTL_ETH_OFWSKIP_GET:
			/* Get the current overflow skip counter. */
			arg = _ne2k_skip_cnt;
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

static int ne2k_open(struct inode *inode, struct file *file)
{
	int err = 0;

	if (ne2k_inuse) {
		err = -EBUSY;
	} else {
		ne2k_reset();
		ne2k_init();
		ne2k_start();

		ne2k_inuse = 1;
	}
	return err;
}

/*
 * Release (close) device
 */

static void ne2k_release(struct inode *inode, struct file *file)
{
	ne2k_stop();

	ne2k_inuse = 0;
}

/*
 * Ethernet operations
 */

static struct file_operations ne2k_fops =
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
	printk("Skip Count %d\n", _ne2k_skip_cnt);
}
#endif

/*
 * Ethernet main initialization (during boot)
 * FIXME: Needs return value to signal that initalization failed.
 */

void ne2k_drv_init(void)
{
	int err, i;
	int is_8bit;	
	word_t prom[16];	/* PROM containing HW MAC address and more 
				 * (aka SAPROM, Station Address PROM).
				 * PROM size is 16 bytes. If read in word (16 bit) mode,
				 * the upper byte is either a copy of the lower or 00.
				 * depending on the chip. This may be used to detect 8 vs 16
				 * bit cards automagically.
				 * The address occupies the first 6 bydes, followed by a 'card signature'.
				 * If bytes 14 & 15 == 0x57, this is a ne2k clone.
				 */
	byte_t *cprom;

	is_8bit = net_port&3;	// Set 8bit mode via /bootopts
				// Bit 0 set: 8 bit interface
				// Bit 1 set: use 16k buffer regardless

/* 
 * DEBUG: use the 2nd nibble of the port # for overflow skip-count
 * obviously works only if the i/o address has zero in this nibble.
 */
	_ne2k_skip_cnt = 1;
#if 0
	i = (net_port&0xf0)>>4;
	if (i) _ne2k_skip_cnt = i;
	net_port &= 0xff0f;
/* ----------------------------------*/
#endif

	net_port &= 0xfffc;
#ifdef CONFIG_ETH_BYTE_ACCESS
	is_8bit = 1;		// Force 8 bit, 4k mode
#endif

	while (1) {
		err = ne2k_probe();
		if (err) {
			printk("eth: NE2K not found at 0x%x, irq %d\n", net_port, net_irq);
			break;
		}
		err = request_irq(net_irq, ne2k_int, INT_GENERIC);
		if (err) {
			printk("eth: NE2K IRQ %d request error: %i\n", net_irq, err);
			break;
		}

		err = register_chrdev(ETH_MAJOR, "eth", &ne2k_fops);
		if (err) {
			printk("eth: register error: %i\n", err);
			break;
		}

		cprom = (byte_t *)prom;

		ne2k_get_hw_addr(prom);

		/* If there is no prom (i.e. emulator), use default MAC */
		/* or we have QEMU which doesn't behave like physical */
		if (((prom[0]&0xff) == 0xff) && ((prom[1]&0xff) == 0xff)) {
			err = -1;
		} else {
			int j, k;
			err = j = k = 0;
			//for (i = 0; i < 32; i++) printk("%02x", cprom[i]);
			//printk("\n");

			// if the high byte of every word is 0, this is a 16 bit card
			// if the high byte = low byte in every word, this is QEMU
			for (i = 1; i < 12; i += 2) j += cprom[i];
			for (i = 0; i < 12; i += 2) k += cprom[i]; // QEMU hack

			if (j && (j!=k)) {	
				//printk("8 bit card detected \n");
				is_8bit |= 1;	// keep 16k override if set
			} else {
				for (i = 0; i < 16; i++) cprom[i] = (char)prom[i]&0xff;
			}
			//for (i = 0; i < 16; i++) printk("%02x", cprom[i]);
			//printk("\n");

		}
		printk ("eth: NE2K (%d bit) at 0x%x, irq %d, ", 16-8*(is_8bit&1), net_port, net_irq);
		if (!err) 	/* address found, interface is present */
			memcpy(mac_addr, cprom, 6);
		else
			printk("No MAC address, using default\n");

		printk ("MAC %02x", mac_addr[0]);
		i = 1;
		while (i < 6) printk(":%02x", mac_addr[i++]);
		printk("\n");
		if (is_8bit&2) printk("eth: Forced 16k buffer mode\n");
		if (!is_8bit) _ne2k_skip_cnt = 3;	// Experimental
		//printk("eth (debug): oflow-strategy %d\n", _ne2k_skip_cnt);

		ne2k_addr_set(cprom);   /* Set NIC mac addr now so IOCTL works */
#if DEBUG_ETH
		debug_setcallback(ne2k_display_status);
#endif
		break;

	}
	net_rx_errs = 0;
	net_tx_errs = 0;
	net_oflow_errs = 0;
	_ne2k_has_data = 0;
	_ne2k_is_8bit = is_8bit;

	return;
}
