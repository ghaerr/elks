//-----------------------------------------------------------------------------
// Ethernet driver
//-----------------------------------------------------------------------------

#include <linuxmt/errno.h>
#include <linuxmt/major.h>
#include <linuxmt/fs.h>


#define PACKET_MAX (6 * 256)


// Static data

static byte_t eth_inuse = 0;

//static struct wait_queue rx_queue;
//static struct wait_queue tx_queue;

//static byte_t recv_buf [PACKET_MAX];
//static byte_t send_buf [PACKET_MAX];

static byte_t echo_buf [PACKET_MAX];


// Get packet

static size_t eth_read (struct inode * inode, struct file * filp,
	char * data, unsigned int len)

	{
	size_t res;

	size_t size;  // actual packet size

	while (1)
		{
		/*
		res = ne2k_pack_get (recv_buf);
		if (res)
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
			*/

		// Client should request at least PACKET_MAX bytes
		// otherwise end of packet will be lost

		//size = *((word_t *) (recv_packet + 2));
		size = PACKET_MAX;
		len = len > size ? size : len;
		//memcpy_tofs (data, recv_packet, len);
		memcpy_tofs (data, echo_buf, len);

		res = len;
		break;
		}

    return res;
	}


// Put packet

static size_t eth_write (struct inode * inode, struct file * filp,
	char * data, size_t len)

	{
	size_t res;

	size_t size;  // actual packet size

	// Client should write packet once
	// otherwise end of packet will be lost

	size = PACKET_MAX;
	len = len > size ? size : len;
	memcpy_fromfs (echo_buf, data, len);

	res = len;
    return res;
	}


// Test for readiness

int eth_select (struct inode * inode, struct file * filp, int sel_type)
	{
    int err = 0;

    switch (sel_type)
    	{
    	case SEL_OUT:
    	    //select_wait (&tx_queue);
    		err = 1;
    		break;

    	case SEL_IN:
    	    //select_wait (&rx_queue);
    		err = 1;
    		break;

    	default:
    		err = -EINVAL;

    	}

    return err;
	}


// Interrupt handler

static void eth_irq (int irq, struct pt_regs * regs, void * dev_id)
	{
	/*
    word_t stat = ne2k_int_stat ();

    if (stat & NE2K_STAT_RD)
    	{
    	wake_up (&rx_queue);
    	}

    if (stat & NE2K_STAT_WR)
    	{
    	wake_up (&tx_queue);
    	}
    */
    }


// I/O control

static int eth_ioctl ()
	{
	return 0;
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

		/*
		err = request_irq (NE2K_IRQ, eth_int, NULL);
		if (err) break;

		ne2k_reset ();

		err = ne2k_init ();
		if (err) break;

		err = ne2k_start ();
		if (err) break;
		*/

		err = 0;
		break;
		}

	return err;
	}


// Release (close)

static void eth_release (struct inode * inode, struct file * file)
	{
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

	err = register_chrdev (ETH_MAJOR, "eth", &eth_fops);
	if (err)
		{
		printk ("Failed to register ethernet driver: %i\n", err);
		}
	}


//-----------------------------------------------------------------------------
