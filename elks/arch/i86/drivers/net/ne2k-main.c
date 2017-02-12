//-----------------------------------------------------------------------------
// Ethernet driver
//-----------------------------------------------------------------------------

#include <linuxmt/errno.h>
#include <linuxmt/major.h>
#include <linuxmt/ioctl.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/fs.h>
#include <linuxmt/sched.h>

#include "ne2k.h"


#define PACKET_MAX (6 * 256)


// Static data

static byte_t eth_inuse = 0;

static byte_t mac_addr [6] = {2, 0, 0, 0, 0, 1};

static struct wait_queue rx_queue;
static struct wait_queue tx_queue;

static byte_t recv_buf [PACKET_MAX];
static byte_t send_buf [PACKET_MAX];


// Get packet

static size_t eth_read (struct inode * inode, struct file * filp,
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

				res = -ERESTARTSYS;
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

		// Client should request at least PACKET_MAX bytes
		// otherwise end of packet will be lost

		size = *((word_t *) (recv_buf + 2));
		len = len > size ? size : len;
		memcpy_tofs (data, recv_buf + 4, len);

		res = len;
		break;
		}

    return res;
	}


// Put packet

static size_t eth_write (struct inode * inode, struct file * file,
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

				res = -ERESTARTSYS;
				break;
				}

			// Try again

			continue;
			}

		// Client should write packet once
		// otherwise end of packet will be lost

		len = len > PACKET_MAX ? PACKET_MAX : len;
		memcpy_fromfs (send_buf, data, len);

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

int eth_select (struct inode * inode, struct file * filp, int sel_type)
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

static void eth_int (int irq, struct pt_regs * regs, void * dev_id)
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

static int eth_ioctl (struct inode * inode, struct file * file,
	unsigned int cmd, unsigned int arg)

	{
	int err;

	switch (cmd)
		{
		case IOCTL_ETH_TEST:
			err = ne2k_test ();
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

void eth_init ()
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

		err = request_irq (NE2K_IRQ, eth_int, NULL);
		if (err)
			{
			printk ("[eth] IRQ 9 request error: %i\n", err);
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
