//-----------------------------------------------------------------------------
// Ethernet driver
//-----------------------------------------------------------------------------

#include <linuxmt/errno.h>
#include <linuxmt/major.h>
#include <linuxmt/ioctl.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/fs.h>
#include <linuxmt/sched.h>
#include <linuxmt/limits.h>
#include <linuxmt/mm.h>


// Shared declarations between low and high parts

#include "ne2k.h"


// Static data

static byte_t eth_inuse = 0;

static byte_t mac_addr [6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};  // QEMU default

static byte_t recv_buf [MAX_PACKET_ETH];
static byte_t send_buf [MAX_PACKET_ETH];


// Receive condition

static bool_t rx_test (void * p)
{
	return (ne2k_rx_stat () == NE2K_STAT_RX);
}

static cond_t rx_cond = { rx_test, NULL };


// Get packet

static size_t eth_read (struct inode * inode, struct file * filp,
	char * data, size_t len)
{
	size_t res;

	while (1) {
		size_t size;  // actual packet size

		if (ne2k_rx_stat () != NE2K_STAT_RX) {

			if (filp->f_flags & O_NONBLOCK) {
				res = -EAGAIN;
				break;
			}

			if (wait_event_interruptible (&rx_cond)) {
				res = -EINTR;  // interrupted by signal
				break;
			}
		}

		if (ne2k_pack_get (recv_buf)) {
			res = -ERESTARTSYS;  // panic
			break;
		}

		// Client should read packet once
		// otherwise end of packet will be lost

		size = *((word_t *) (recv_buf + 2));
		if (len > size) len = size;
		memcpy_tofs (data, recv_buf + 4, len);

		res = len;
		break;
	}

	return res;
}


// Transmit condition

static bool_t tx_test (void * p)
{
	return (ne2k_tx_stat () == NE2K_STAT_TX);
}

static cond_t tx_cond = { tx_test, NULL };


// Put packet

static size_t eth_write (struct inode * inode, struct file * file,
	char * data, size_t len)
{
	size_t res;

	while (1) {
		if (ne2k_tx_stat () != NE2K_STAT_TX) {

			if (file->f_flags & O_NONBLOCK) {
				res = -EAGAIN;
				break;
			}

			if (wait_event_interruptible (&tx_cond)) {
				res = -EINTR;  // interrupted by signal
				break;
			}
		}

		// Client should write packet once
		// otherwise end of packet will be lost

		if (len > MAX_PACKET_ETH) len = MAX_PACKET_ETH;
		memcpy_fromfs (send_buf, data, len);

		if (len < 64) len = 64;  /* issue #133 */
		if (ne2k_pack_put (send_buf, len)) {
			res = -ERESTARTSYS;  // panic
			break;
		}

		res = len;
		break;
	}

	return res;
}


// Test for readiness

int eth_select (struct inode * inode, struct file * filp, int sel_type)
{
	int res = 0;

	switch (sel_type) {
		case SEL_OUT:
			if (ne2k_tx_stat () != NE2K_STAT_TX)
			{
				select_wait ((struct wait_queue *) &tx_cond);
				break;
			}

			res = 1;
			break;

		case SEL_IN:
			if (ne2k_rx_stat () != NE2K_STAT_RX)
			{
				select_wait ((struct wait_queue *) &rx_cond);
				break;
			}

			res = 1;
			break;

		default:
			res = -EINVAL;

	}

	return res;
}


// Interrupt handler

static void ne2k_int (int irq, struct pt_regs * regs, void * dev_id)
{
	word_t stat;

	stat = ne2k_int_stat ();
	if (stat & NE2K_STAT_RX)
		wake_up ((struct wait_queue *) &rx_cond);

	if (stat & NE2K_STAT_TX)
		wake_up ((struct wait_queue *) &tx_cond);
}


// I/O control

static int eth_ioctl (struct inode * inode, struct file * file,
	unsigned int cmd, unsigned int arg)

	{
	int err = 0;

	switch (cmd)
		{
		case IOCTL_ETH_TEST:
			err = ne2k_test ();
			break;

		case IOCTL_ETH_ADDR_GET:
			memcpy_tofs ((char *) arg, mac_addr, 6);
			break;

		case IOCTL_ETH_ADDR_SET:
			memcpy_fromfs (mac_addr, (char *) arg, 6);
			ne2k_addr_set (mac_addr);
			break;

		default:
			err = -EINVAL;

		}

	return err;
	}


// Open

static int eth_open (struct inode * inode, struct file * file)
	{
	int err;

	while (1)
		{
		if (eth_inuse)
			{
			err = -EBUSY;
			break;
			}

		ne2k_reset ();

		err = ne2k_init ();
		if (err) break;

		ne2k_addr_set (mac_addr);

		err = ne2k_start ();
		if (err) break;

		eth_inuse = 1;

		err = 0;
		break;
		}

	return err;
	}


// Release (close)

static void eth_release (struct inode * inode, struct file * file)
	{
	ne2k_stop ();
	ne2k_term ();

	eth_inuse = 0;
	}


// Ethernet operations

static struct file_operations eth_fops =
	{
	NULL,         /* lseek */
    eth_read,
    eth_write,
    NULL,         /* readdir */
    eth_select,
    eth_ioctl,
    eth_open,
    eth_release

#ifdef BLOAT_FS
    NULL,         /* fsync */
    NULL,         /* check_media_type */
    NULL          /* revalidate */
#endif

	};


// Ethernet main initialization

void eth_drv_init ()
	{
	int err;

	while (1)
		{
		err = ne2k_probe ();
		if (err)
			{
			printk ("[ne2k] not detected @ IO 300h\n");
			break;
			}

		err = request_irq (NE2K_IRQ, ne2k_int, NULL);
		if (err)
			{
			printk ("[ne2k] IRQ 9 request error: %i\n", err);
			break;
			}

		err = register_chrdev (ETH_MAJOR, "eth", &eth_fops);
		if (err)
			{
			printk ("[eth] register error: %i\n", err);
			break;
			}

		printk ("[eth] NE2K driver OK\n");
		break;
		}
	}


//-----------------------------------------------------------------------------
