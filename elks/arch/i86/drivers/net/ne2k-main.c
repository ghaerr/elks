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


// Shared declarations between low and high parts

#include "ne2k.h"


// Static data

static byte_t ne2k_inuse = 0;

static byte_t mac_addr [6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};  // QEMU default

static struct wait_queue rx_queue;
static struct wait_queue tx_queue;

static byte_t recv_buf [MAX_PACKET_ETH];
static byte_t send_buf [MAX_PACKET_ETH];


// Get packet

static size_t ne2k_read (struct inode * inode, struct file * filp,
	char * data, unsigned int len)

	{
	size_t res;

	while (1)
		{
		size_t size;  // actual packet size

		res = ne2k_rx_stat ();
		if (res != NE2K_STAT_RX)
			{
			// No packet received

			if (filp->f_flags & O_NONBLOCK)
				{
				res = -EAGAIN;
				break;
				}

			interruptible_sleep_on (&rx_queue);
			if (current->signal)
				{
				// Interrupted by signal

				res = -EINTR;
				break;
				}

			// Try again

			continue;
			}

		res = ne2k_pack_get (recv_buf);
		if (res)
			{
			// Panic

			res = -ERESTARTSYS;
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


// Put packet

static size_t ne2k_write (struct inode * inode, struct file * file,
	char * data, size_t len)

	{
	size_t res;

	while (1)
		{
		res = ne2k_tx_stat ();
		if (res != NE2K_STAT_TX)
			{
			// Not ready to send

			if (file->f_flags & O_NONBLOCK)
				{
				res = -EAGAIN;
				break;
				}

			interruptible_sleep_on (&tx_queue);
			if (current->signal)
				{
				// Interrupted by signal

				res = -EINTR;
				break;
				}

			// Try again

			continue;
			}

		// Client should write packet once
		// otherwise end of packet will be lost

		if (len > MAX_PACKET_ETH) len = MAX_PACKET_ETH;
		memcpy_fromfs (send_buf, data, len);

		if (len < 64) len = 64;  /* issue #133 */
		res = ne2k_pack_put (send_buf, len);
		if (res)
			{
			res = -ERESTARTSYS;
			break;
			}

		res = len;
		break;
		}

	return res;
	}


// Test for readiness

int ne2k_select (struct inode * inode, struct file * filp, int sel_type)
	{
	int res = 0;

	switch (sel_type)
		{
		case SEL_OUT:
			if (ne2k_tx_stat () != NE2K_STAT_TX)
				{
				select_wait (&tx_queue);
				break;
				}

			res = 1;
			break;

		case SEL_IN:
			if (ne2k_rx_stat () != NE2K_STAT_RX)
				{
				select_wait (&rx_queue);
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
    	{
    	wake_up (&rx_queue);
    	}

    if (stat & NE2K_STAT_TX)
    	{
    	wake_up (&tx_queue);
    	}
    }


// I/O control

static int ne2k_ioctl (struct inode * inode, struct file * file,
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

static int ne2k_open (struct inode * inode, struct file * file)
	{
	int err;

	while (1)
		{
		if (ne2k_inuse)
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

		ne2k_inuse = 1;

		err = 0;
		break;
		}

	return err;
	}


// Release (close)

static void ne2k_release (struct inode * inode, struct file * file)
	{
	ne2k_stop ();
	ne2k_term ();

	ne2k_inuse = 0;
	}


// Ethernet operations

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


// Ethernet main initialization

void ne2k_drv_init ()
	{
	int err;

	while (1)
		{
		err = ne2k_probe ();
		if (err)
			{
			printk ("[eth] NE2K not detected @ IO 300h\n");
			break;
			}

		err = request_irq (NE2K_IRQ, ne2k_int, NULL);
		if (err)
			{
			printk ("[eth] IRQ 9 request error: %i\n", err);
			break;
			}

		err = register_chrdev (ETH_MAJOR, "eth", &ne2k_fops);
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
