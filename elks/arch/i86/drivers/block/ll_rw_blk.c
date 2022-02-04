/*
 *  linux/drivers/block/ll_rw_blk.c
 *
 * Copyright (C) 1991, 1992 Linus Torvalds
 * Copyright (C) 1994,      Karl Keyte: Added support for disk statistics
 */

/*
 * This handles all read/write requests to block devices
 */

#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/fs.h>
#include <linuxmt/errno.h>
#include <linuxmt/string.h>
#include <linuxmt/config.h>
#include <linuxmt/init.h>
#include <linuxmt/mm.h>
#include <linuxmt/debug.h>
#include <arch/system.h>
#include <arch/io.h>
#include <arch/irq.h>

#include "blk.h"

#ifdef CONFIG_ROMFS_FS
#include "romflash.h"
#endif

/*
 * The request-struct contains all necessary data
 * to load a number of sectors into memory
 *
 * NR_REQUEST is the number of entries in the request-queue.
 * NOTE that writes may use only the low 2/3 of these: reads
 * take precedence.
 */

#define NR_REQUEST	20

static struct request all_requests[NR_REQUEST];

/*
 * blk_dev_struct is:
 *	*request_fn
 *	*current_request
 */

struct blk_dev_struct blk_dev[MAX_BLKDEV];	/* initialized by blk_dev_init() */

/*
 * blk_size contains the size of all block-devices in units of 1024 byte
 * sectors:
 *
 * blk_size[MAJOR][MINOR]
 *
 * if (!blk_size[MAJOR]) then no minor size checking is done.
 */

#ifdef BDEV_SIZE_CHK
int *blk_size[MAX_BLKDEV] = { NULL, NULL, };
#endif

/*
 * blksize_size contains the size of all block-devices:
 *
 * blksize_size[MAJOR]
 *
 */

/* int blksize_size[MAX_BLKDEV] = {0,}; */

/*
 * hardsect_size contains the size of the hardware sector of a device.
 *
 * hardsect_size[MAJOR][MINOR]
 *
 * if (!hardsect_size[MAJOR])
 *		then 512 bytes is assumed.
 * else
 *		sector_size is hardsect_size[MAJOR][MINOR]
 *
 * This is currently set by some scsi device and read by the msdos fs driver
 * This might be a some uses later.
 */

/* int * hardsect_size[MAX_BLKDEV] = { NULL, NULL, }; */

/*
 * look for a free request in the first N entries.
 * NOTE: interrupts must be disabled on the way in, and will still
 *       be disabled on the way out.
 */
static struct request *get_request(int n, kdev_t dev)
{
    static struct request *prev_found = NULL;
    static struct request *prev_limit = NULL;
    register struct request *req;
    register struct request *limit;

    limit = all_requests + n;
    if (limit != prev_limit) {
	prev_limit = limit;
	prev_found = all_requests;
    }
    req = prev_found;
    do {
	req = ((req > all_requests) ? req : limit) - 1;
	if (req->rq_status == RQ_INACTIVE) {
	    prev_found = req;
	    req->rq_status = RQ_ACTIVE;
	    req->rq_dev = dev;
	    return req;
	}
    } while (req != prev_found);
    return NULL;
}

#if 0
static struct request *get_request_wait(int n, kdev_t dev)
{
    register struct request *req;

    clr_irq();
    req = get_request(n, dev);
    set_irq();
    if (req)
	return req;
    return __get_request_wait(n, dev);
}
#endif

/*
 * add-request adds a request to the linked list.
 * It disables interrupts so that it can muck with the
 * request-lists in peace.
 */

static void add_request(struct blk_dev_struct *dev, struct request *req)
{
    register struct request *tmp;

    clr_irq();
    mark_buffer_clean(req->rq_bh);
    if (!(tmp = dev->current_request)) {
	dev->current_request = req;
	set_irq();
	(dev->request_fn) ();
    }
    else {
	for (; tmp->rq_next; tmp = tmp->rq_next) {
	    if ((IN_ORDER(tmp, req) ||
		!IN_ORDER(tmp, tmp->rq_next)) && IN_ORDER(req, tmp->rq_next))
		break;
	}
	req->rq_next = tmp->rq_next;
	tmp->rq_next = req;
	set_irq();
    }
}

static void make_request(unsigned short major, int rw, struct buffer_head *bh)
{
    struct request *req;
    int max_req;

    debug_blk("BLK %lu %s %lx:%x\n", bh->b_blocknr, rw==READ? "read": "write",
	bh->b_seg, buffer_data(bh));

#ifdef BDEV_SIZE_CHK
    sector_t count = BLOCK_SIZE / SECTOR_SIZE;	/* FIXME must move to lower level*/
    sector_t sector = bh->b_blocknr * count;
    if (blk_size[major])
	if (blk_size[major][MINOR(bh->b_dev)] < (sector + count) >> 1) {
	    printk("attempt to access beyond end of device\n");
	    return;
	}
#endif

    /* Uhhuh.. Nasty dead-lock possible here.. */
    if (buffer_locked(bh))
	return;
    /* Maybe the above fixes it, and maybe it doesn't boot. Life is interesting */
    lock_buffer(bh);

    switch (rw) {

    case READ:
	max_req = NR_REQUEST;	/* reads take precedence */
	break;

    case WRITE:
	/* We don't allow the write-requests to fill up the
	 * queue completely:  we want some room for reads,
	 * as they take precedence. The last third of the
	 * requests are only for reads.
	 */
	max_req = (NR_REQUEST * 2) / 3;
	break;

    default:
	debug("make_request: bad block dev cmd, must be R/W/RA/WA\n");
	unlock_buffer(bh);
	return;
    }

    /* find an unused request. */
    req = get_request(max_req, bh->b_dev);
    set_irq();

	// Try again blocking if no request available

    if (!req) {

/* I suspect we may need to call get_request_wait() but not at the moment
 * For now I will wait until we start getting panics, and then work out
 * what we have to do - Al <ajr@ecs.soton.ac.uk>
 */

#ifdef MULTI_BH
	req = __get_request_wait(max_req, bh->b_dev);
#else
	panic("Can't get request.");
#endif

    }

    /* fill up the request-info, and add it to the queue */
    req->rq_cmd = (__u8) rw;
    req->rq_blocknr = bh->b_blocknr;
    req->rq_seg = bh->b_seg;
    req->rq_buffer = buffer_data(bh);
    req->rq_bh = bh;

#ifdef BLOAT_FS
    req->rq_nr_sectors = count;
    req->rq_current_nr_sectors = count;
#endif

    req->rq_next = NULL;
    add_request(&blk_dev[major], req);
}


#ifdef MULTI_BH
struct wait_queue wait_for_request = NULL;

/*
 * wait until a free request in the first N entries is available.
 */
static struct request *__get_request_wait(int n, kdev_t dev)
{
    register struct request *req;
    printk("Waiting for request...\n");

    wait_set(&wait_for_request);
    current->state = TASK_UNINTERRUPTIBLE;
    goto startgrw;
    do {
	schedule();
      startgrw:
#if 0
	unplug_device(MAJOR(dev) + blk_dev);	/* Device can't be plugged */
#endif
	clr_irq();
	req = get_request(n, dev);
	set_irq();
    } while (req == NULL);
    current->state = TASK_RUNNING;
    wait_clear(&wait_for_request);

    return req;
}

/*
 * "plug" the device if there are no outstanding requests: this will
 * force the transfer to start only after we have put all the requests
 * on the list.
 */

static void plug_device(register struct blk_dev_struct *dev,
			struct request *plug)
{
    flag_t flags;

    plug->rq_status = RQ_INACTIVE;
    plug->cmd = -1;
    plug->next = NULL;
    save_flags(flags);
    clr_irq();
    if (!dev->current_request)
	dev->current_request = plug;
    restore_flags(flags);
}

/*
 * remove the plug and let it rip..
 */

static void unplug_device(struct blk_dev_struct *dev)
{
    register struct request *req;
    unsigned int flags;

    save_flags(flags);
    clr_irq();
    req = dev->current_request;
    if (req && req->rq_status == RQ_INACTIVE && req->cmd == -1) {
	dev->current_request = req->next;
	(dev->request_fn) ();
    }
    restore_flags(flags);
}

/* This function can be used to request a number of buffers from a block
 * device. Currently the only restriction is that all buffers must belong
 * to the same device.
 */
void ll_rw_block(int rw, int nr, register struct buffer_head **bh)
{
    struct blk_dev_struct *dev;
    struct request plug;
    unsigned short int major;
    int i;

    /* Make sure the first block contains something reasonable */
    while (!*bh) {
	bh++;
	if (--nr <= 0)
	    return;
    }
    dev = NULL;
    if ((major = MAJOR(bh[0]->b_dev)) < MAX_BLKDEV)
	dev = blk_dev + major;
    if (!dev || !dev->request_fn) {
	printk("ll_rw_block: Trying to read nonexistent block-device %s (%lu)\n",
	     kdevname(bh[0]->b_dev), bh[0]->b_blocknr);
	goto sorry;
    }

    /* If there are no pending requests for this device, then we insert
     * a dummy request for that device.  This will prevent the request
     * from starting until we have shoved all of the blocks into the
     * queue, and then we let it rip.  */
    if (nr > 1)
	plug_device(dev, &plug);
    for (i = 0; i < nr; i++)
	if (bh[i])
	    make_request(major, rw, bh[i]);
    unplug_device(dev);
    return;

  sorry:
    for (i = 0; i < nr; i++)
	if (bh[i]) {
	    bh[i]->b_dirty = 0;
	    bh[i]->b_uptodate = 0;
	}
}
#endif /* MULTI_BH */

/* This function can be used to request a single buffer from a block device.
 */

void ll_rw_blk(int rw, register struct buffer_head *bh)
{
    register struct blk_dev_struct *dev;
    unsigned short int major;

    dev = NULL;
    if ((major = MAJOR(bh->b_dev)) < MAX_BLKDEV)
	dev = blk_dev + major;
    if (!dev || !dev->request_fn) {
	printk("ll_rw_blk: Trying to read nonexistent block-device %s (%lu)\n",
	       kdevname(bh->b_dev), bh->b_blocknr);
	bh->b_dirty = 0;
	bh->b_uptodate = 0;
    } else
	make_request(major, rw, bh);
}

void INITPROC blk_dev_init(void)
{
    register struct request *req;
    register struct blk_dev_struct *dev;

    dev = blk_dev;
    do {
	dev->request_fn = NULL;
	dev->current_request = NULL;
    } while (++dev < &blk_dev[MAX_BLKDEV]);

    req = all_requests;
    do {
	req->rq_status = RQ_INACTIVE;
	req->rq_next = NULL;
    } while (++req < &all_requests[NR_REQUEST]);

#ifdef CONFIG_BLK_DEV_RAM
    rd_init();		/* RAMDISK block device*/
#endif

#if defined(CONFIG_BLK_DEV_SSD_TEST) || defined(CONFIG_BLK_DEV_SSD_SD8018X)
    ssd_init();		/* SSD block device*/
#endif

#ifdef CONFIG_BLK_DEV_HD
    directhd_init();
#endif

#ifdef CONFIG_BLK_DEV_FD
    floppy_init();
#endif

#ifdef CONFIG_BLK_DEV_BIOS
    bioshd_init();
#endif

#ifdef CONFIG_ROMFS_FS
    romflash_init ();
#endif
}
