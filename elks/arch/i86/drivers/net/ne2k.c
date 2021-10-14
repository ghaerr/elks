/*
 *
 * NE2K Ethernet driver - supports NICs using the NS DP8390 and
 * compatible chip sets. Tested with Eagle 8390 (6321839050001 (PnP) and
 * Winbond W89C902P based cards.
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

// Static data
struct wait_queue rxwait;
struct wait_queue txwait;

static byte_t ne2k_inuse = 0;

static byte_t mac_addr[6]  = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};  /* QEMU default */
			     /* Overwritten by actual HW MAC address if found */

extern word_t _ne2k_has_data;
extern word_t _ne2k_skip_cnt;	/* If the NIC ring buffer overflows, skip this # of packets,
				 * zero means 'all'. */

/*
 * Get packet
 */

static size_t ne2k_read(struct inode * inode, struct file * filp, char * data, size_t len)
{
	size_t res;
	word_t nhdr[2];	/* packet header from the NIC, for debugging */

	while (1) {
		size_t size;  // actual packet size

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

		debug_eth("eth read: req %d, got %d real %d\n", len, size, nhdr[1]);
		if (nhdr[1] > size)
			printk("eth: truncated read - %d %d\n", nhdr[1], size);

		res = size;
		break;
	}

	finish_wait(&rxwait);
	return res;
}

/*
 * Pass packet to driver for send
 */

static size_t ne2k_write (struct inode * inode, struct file * file, char * data, size_t len)
{
	size_t res;

	while (1) {
		prepare_to_wait_interruptible(&txwait);
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
		if (ne2k_pack_put(data, len)) {
			res = -EIO;
			break;
		}

		res = len;
		break;
	}

	finish_wait(&txwait);
	return res;
}

/*
 * Test for readiness
 */

int ne2k_select(struct inode * inode, struct file * filp, int sel_type)
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

static void ne2k_int(int irq, struct pt_regs * regs)
{
	word_t stat, page;

	stat = ne2k_int_stat();
#if 0
	printk("/%d/", stat);
	page = ne2k_getpage();
	printk("|%04x|", page);
#endif

        if (stat & NE2K_STAT_OF) {
		printk("eth: NIC receive oflow (0x%x), ", stat);
		if (_ne2k_skip_cnt) 
			printk("skipping %d packets.\n", _ne2k_skip_cnt);
		else
			printk("clearing buffer.\n");
		page = ne2k_clr_oflow(); 
		debug_eth("/CB%04x/ ", page);
		/* The clr_oflow routine effectively resets the NIC, clears
		 * the ISR, so more processing of interrupt status bits is
		 * meaningless. */
		return;
	}

	if (stat & NE2K_STAT_RX) {
		_ne2k_has_data = 1; 
		wake_up(&rxwait);
	}

	if (stat & NE2K_STAT_TX) {
		wake_up(&txwait);
	}
	if (stat & NE2K_STAT_RDC) {
		printk("eth: Warning - RDC intr. (0x%x)\n", stat);
		/* The RDC interrupt should be disabled in the low level driver.
		 * When real DMA transfer from NIC to system RAM is enabled, this is where
		 * we handle transfer completion.
		 * NOTICE: If we get here, a remote DMA transfer was aborted. This should
		 * not happen.
		 */
		ne2k_rdc();
	}
	if (stat & NE2K_STAT_TXE) { 	
		/* transmit error detected, this should not happen. */
		printk("eth: TX-error, status 0x%x\n", ne2k_get_tx_stat());
		/* ne2k_get_tx_stat resets this int in the ISR */
	}
	debug_eth("%02X/%d/",stat,_ne2k_has_data);
	stat = page; /* to keep compiler happy */
}

/*
 * I/O control
 */

static int ne2k_ioctl(struct inode * inode, struct file * file, unsigned int cmd, unsigned int arg)
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
			 * arg must be a 32 bytes array.
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

static int ne2k_open(struct inode * inode, struct file * file)
{
	int err;

	while (1) {
		if (ne2k_inuse) {
			err = -EBUSY;
			break;
		}

		ne2k_reset();
		ne2k_init();
		ne2k_start();

		ne2k_inuse = 1;
		err = 0;
		break;
	}
	return err;
}

/*
 * Release (close) device
 */

static void ne2k_release(struct inode * inode, struct file * file)
{
	ne2k_stop();
	ne2k_term();

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

#ifdef BLOAT_FS
    NULL,         /* fsync */
    NULL,         /* check_media_type */
    NULL          /* revalidate */
#endif
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
	int err;
	word_t prom[16];	/* PROM containing HW MAC address and more 
				 * (aka SAPROM, Station Address PROM).
				 * If byte 14 & 15 == 0x57, this is a ne2k clone.
				 * May be used to avoid attempts to use an unsupported NIC
				 * PROM size is 32 bytes.
				 */
	byte_t hw_addr[6];

	while (1) {
		err = ne2k_probe();
		if (err) {
			printk ("eth: NE2K not found @ IRQ %d, IO 0x%x\n", net_irq, net_port);
			break;
		}
		err = request_irq (net_irq, ne2k_int, INT_GENERIC);
		if (err) {
			printk ("eth: NE2K IRQ %d request error: %i\n", net_irq, err);
			break;
		}

		err = register_chrdev (ETH_MAJOR, "eth", &ne2k_fops);
		if (err) {
			printk ("eth: register error: %i\n", err);
			break;
		}

		int i = 0;

		ne2k_get_hw_addr(prom);

		/* If there is no prom (i.e. emulator), use default */
		if (((prom[0]&0xff) == 0xff) && ((prom[1]&0xff) == 0xff)) {
			err = -1;
		} else {
			while (i < 6) hw_addr[i] = prom[i]&0xff,i++;
			err = 0;
		}

		printk ("eth: NE2K at 0x%x, irq %d, ", net_port, net_irq);
		if (!err) {	/* address found, interface is present */

			printk ("MAC %02x", hw_addr[0]);
			i = 1;
			while (i < 6) printk(":%02x", hw_addr[i++]);
			printk("\n");

			memcpy(mac_addr, hw_addr, 6);
			ne2k_addr_set(hw_addr);   /* Set NIC mac addr now so IOCTL works */
#if DEBUG_ETH
			debug_setcallback(ne2k_display_status);
#endif
		} else
			printk("not responding.\n");

		break;

	}
	_ne2k_has_data = 0;
	_ne2k_skip_cnt = 0;	/* # of packets to discard if the NIC buffer overflows. 
                 		 * Zero is the default, discard entire buffer less one pkt.
				 * May be changed via ioctl.
				 * A big # will clear the entire buffer.
				 * On a floppy based system, anything else is useless.
				 */
	return;
}
