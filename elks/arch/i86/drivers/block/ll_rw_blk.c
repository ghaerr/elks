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
/*#include <linux/locks.h>*/
#include <linuxmt/mm.h>

#include <arch/system.h>
#include <arch/io.h>
#include <arch/irq.h>
#include "blk.h"

/*
 * The request-struct contains all necessary data
 * to load a nr of sectors into memory
 *
 * NR_REQUEST is the number of entries in the request-queue.
 * NOTE that writes may use only the low 2/3 of these: reads
 * take precedence.
 */
 
#define NR_REQUEST	20

static struct request all_requests[NR_REQUEST];

/*
 * used to wait on when there are no free requests
 */

int foo_foo_bar;

#ifdef MULTI_BH
struct wait_queue * wait_for_request = NULL;
#endif

/* This specifies how many sectors to read ahead on the disk.  */

/* int read_ahead[MAX_BLKDEV] = {0, }; */

/* blk_dev_struct is:
 *	*request_fn
 *	*current_request
 */

struct blk_dev_struct blk_dev[MAX_BLKDEV]; /* initialized by blk_dev_init() */

/*
 * blk_size contains the size of all block-devices in units of 1024 byte
 * sectors:
 *
 * blk_size[MAJOR][MINOR]
 *
 * if (!blk_size[MAJOR]) then no minor size checking is done.
 */

#ifdef BDEV_SIZE_CHK
int * blk_size[MAX_BLKDEV] = { NULL, NULL, };
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
 * This is currently set by some scsi device and read by the msdos fs driver
 * This might be a some uses later.
 */

/* int * hardsect_size[MAX_BLKDEV] = { NULL, NULL, }; */

#ifdef MULTI_BH
/*
 * "plug" the device if there are no outstanding requests: this will
 * force the transfer to start only after we have put all the requests
 * on the list.
 */

static void plug_device(dev,plug)
register struct blk_dev_struct * dev;
struct request * plug;
{
	flag_t flags;

	plug->rq_status = RQ_INACTIVE;
	plug->cmd = -1;
	plug->next = NULL;
	save_flags(flags);
	icli();
	if (!dev->current_request)
		dev->current_request = plug;
	restore_flags(flags);
}

/*
 * remove the plug and let it rip..
 */

static void unplug_device(dev)
struct blk_dev_struct * dev;
{
	register struct request * req;
	unsigned int flags;

	save_flags(flags);
	icli();
	req = dev->current_request;
	if (req && req->rq_status == RQ_INACTIVE && req->cmd == -1) {
		dev->current_request = req->next;
		(dev->request_fn)();
	}
	restore_flags(flags);
}
#endif

/*
 * look for a free request in the first N entries.
 * NOTE: interrupts must be disabled on the way in, and will still
 *       be disabled on the way out.
 */
static struct request * get_request(n,dev)
int n;
kdev_t dev;
{
	static struct request *prev_found = NULL;
	static struct request *prev_limit = NULL;
	register struct request *req;
	register struct request *limit;

#ifdef BLOAT_FS
	/* This function is called with a constant value for n */
	if (n <= 0)
		panic("get_request(%d): impossible!\n", n);
#endif

	limit = all_requests + n;
	if (limit != prev_limit) {
		prev_limit = limit;
		prev_found = all_requests;
	}
	req = prev_found;
	for (;;) {
		req = ((req > all_requests) ? req : limit) - 1;
		if (req->rq_status == RQ_INACTIVE)
			break;
		if (req == prev_found)
			return NULL;
	}
	prev_found = req;
	req->rq_status = RQ_ACTIVE;
	req->rq_dev = dev;
	return req;
}

/*
 * wait until a free request in the first N entries is available.
 */

#ifdef MULTI_BH 
static struct request * __get_request_wait(n,dev)
int n;
kdev_t dev;
{
	register struct request *req;
	struct wait_queue wait;
	
	wait.task = current;
	wait.next = NULL;

	printk("Waiting for request...\n");

	add_wait_queue(&wait_for_request, &wait);
	for (;;) {
/*		unplug_device(MAJOR(dev)+blk_dev); */ /* Device can't be plugged */
		current->state = TASK_UNINTERRUPTIBLE;
		icli();
		req = get_request(n, dev);
		isti();
		if (req)
			break;
		schedule();
	}
	remove_wait_queue(&wait_for_request, &wait);
	current->state = TASK_RUNNING;
	return req;
}
#endif

#if 0
static struct request * get_request_wait(n,dev)
int n;
kdev_t dev;
{
	register struct request *req;

	icli();
	req = get_request(n, dev);
	isti();
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

static void add_request(dev,req)
struct blk_dev_struct * dev;
register struct request * req;
{
	register struct request * tmp;
	short		 disk_index;

	icli();
	mark_buffer_clean(req->rq_bh);
	if (!(tmp = dev->current_request)) {
		dev->current_request = req;
		(dev->request_fn)();
		isti();
		return;
	}
	for ( ; tmp->rq_next ; tmp = tmp->rq_next) {
		if ((IN_ORDER(tmp,req) ||
		    !IN_ORDER(tmp,tmp->rq_next)) &&
		    IN_ORDER(req,tmp->rq_next))
			break;
	}
	req->rq_next = tmp->rq_next;
	tmp->rq_next = req;

	isti();
}

static void make_request(major,rw,bh)
int major;
int rw;
register struct buffer_head * bh;
{
	sector_t sector, count;
	register struct request * req;
	int max_req;
#ifdef READ_AHEAD
	int rw_ahead;
#endif


#ifdef BLOAT_FS
	count = bh->b_size >> 9;
#else
	count = BLOCK_SIZE >> 9;
#endif
	sector = bh->b_blocknr * count;
#ifdef BDEV_SIZE_CHK
	if (blk_size[major])
		if (blk_size[major][MINOR(bh->b_dev)] < (sector + count)>>1) {
#ifdef BLOAT_FS
			bh->b_state = 0;
#endif
			printk("attempt to access beyond end of device\n");
			return;
		}
#endif
	/* Uhhuh.. Nasty dead-lock possible here.. */
	if (buffer_locked(bh))
		return;
	/* Maybe the above fixes it, and maybe it doesn't boot. Life is interesting */
	bh->b_lock = 1;
	map_buffer(bh);

#ifdef READ_AHEAD
	rw_ahead = 0;	/* normal case; gets changed below for READA/WRITEA */
#endif
	switch (rw) {
#ifdef READ_AHEAD
		case READA:
			rw_ahead = 1;
			rw = READ;	/* drop into READ */
#endif
		case READ:
			if (buffer_uptodate(bh)) {
				unlock_buffer(bh); /* Hmmph! Already have it */
				return;
			}
			max_req = NR_REQUEST;	/* reads take precedence */
			break;
#ifdef READ_AHEAD
		case WRITEA:
			rw_ahead = 1;
			rw = WRITE;	/* drop into WRITE */
#endif
		case WRITE:
			if (buffer_clean(bh)) {
				unlock_buffer(bh); /* Hmmph! Nothing to write */
				return;
			}
			/* We don't allow the write-requests to fill up the
			 * queue completely:  we want some room for reads,
			 * as they take precedence. The last third of the
			 * requests are only for reads.
			 */
			max_req = (NR_REQUEST * 2) / 3;
			break;
		default:
/*			printk("make_request: bad block dev cmd, must be R/W/RA/WA\n"); */
			unlock_buffer(bh);
			return;
	}

/* find an unused request. */
	req = get_request(max_req, bh->b_dev);
	isti();

/* if no request available: if rw_ahead, forget it; otherwise try again blocking.. */
	if (!req) {
#ifdef READ_AHEAD
		if (rw_ahead) {
			unlock_buffer(bh);
			return;
		}
#endif

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
	req->rq_cmd = rw;
	req->rq_sector = sector;
#ifdef BLOAT_FS
	req->rq_nr_sectors = count;
#endif
	req->rq_buffer = bh->b_data;
/*	req->rq_sem = NULL;*/
	req->rq_bh = bh;
#ifdef BLOAT_FS
	req->rq_current_nr_sectors = count;
	req->rq_errors = 0;
#endif
	req->rq_next = NULL;
	add_request(major+blk_dev,req);
}


/* This function can be used to request a number of buffers from a block
   device. Currently the only restriction is that all buffers must belong to
   the same device */

#ifdef MULTI_BH
void ll_rw_block(rw,nr,bh)
int rw;
int nr;
register struct buffer_head * bh[];
{
	unsigned int major;
	struct request plug;
#ifdef BLOAT_FS
	int correct_size;
#endif
	struct blk_dev_struct * dev;
	int i;

	/* Make sure that the first block contains something reasonable */
	while (!*bh) {
		bh++;
		if (--nr <= 0)
			return;
	};

	dev = NULL;
	if ((major = MAJOR(bh[0]->b_dev)) < MAX_BLKDEV)
		dev = blk_dev + major;
	if (!dev || !dev->request_fn) {
		printk("ll_rw_block: Trying to read nonexistent block-device %s (%ld)\n",
		kdevname(bh[0]->b_dev), bh[0]->b_blocknr);
		goto sorry;
	}

#ifdef BLOAT_FS
	/* Determine correct block size for this device.  */
	correct_size = BLOCK_SIZE;

	/* Verify requested block sizes.  */
	for (i = 0; i < nr; i++) {
		if (bh[i] && bh[i]->b_size != correct_size) {
			printk("ll_rw_block: device %s: only %d-char blocks implemented (%d)\n",
			       kdevname(bh[0]->b_dev),
			       correct_size, bh[i]->b_size);
			goto sorry;
		}
	}
#endif

	/* If there are no pending requests for this device, then we insert
	   a dummy request for that device.  This will prevent the request
	   from starting until we have shoved all of the blocks into the
	   queue, and then we let it rip.  */
	if (nr > 1)
		plug_device(dev, &plug);
	for (i = 0; i < nr; i++) {
		if (bh[i]) {
			make_request(major, rw, bh[i]);
		}
	}
	unplug_device(dev);
	return;

sorry:
	for (i = 0; i < nr; i++) {
		if (bh[i]) {
			bh[i]->b_dirty = 0;
			bh[i]->b_uptodate = 0;
		}
	}
	return;
}
#endif

/* This function can be used to request a single buffer from a block
   device. */

void ll_rw_blk(rw,bh)
int rw;
register struct buffer_head * bh;
{
	unsigned int major;
	struct request plug;
#ifdef BLOAT_FS
	int correct_size;
#endif
	struct blk_dev_struct * dev;

	dev = NULL;
	if ((major = MAJOR(bh->b_dev)) < MAX_BLKDEV)
		dev = blk_dev + major;
	if (!dev || !dev->request_fn) {
		printk("ll_rw_blk: Trying to read nonexistent block-device %s (%ld)\n",
		kdevname(bh->b_dev), bh->b_blocknr);
		goto sorry;
	}

#ifdef BLOAT_FS
	/* Determine correct block size for this device.  */
	correct_size = BLOCK_SIZE;

	/* Verify requested block sizes.  */
	if (bh && bh->b_size != correct_size) {
		printk("ll_rw_blk: device %s: only %d-char blocks implemented (%d)\n",
		       kdevname(bh->b_dev),
		       correct_size, bh->b_size);
		goto sorry;
	}
#endif

	/* If there are no pending requests for this device, then we insert
	   a dummy request for that device.  This will prevent the request
	   from starting until we have shoved all of the blocks into the
	   queue, and then we let it rip.  */
	make_request(major, rw, bh);
	return;
sorry:
	bh->b_dirty = 0;
	bh->b_uptodate = 0;
	return;
}


int blk_dev_init()
{
	register struct request * req;
	register struct blk_dev_struct *dev;

	for (dev = blk_dev + MAX_BLKDEV; dev-- != blk_dev;) {
		dev->request_fn      = NULL;
		dev->current_request = NULL;
	}

	req = all_requests + NR_REQUEST;
	while (--req >= all_requests) {
		req->rq_status = RQ_INACTIVE;
		req->rq_next = NULL;
	}
#ifdef CONFIG_BLK_DEV_RAM
	rd_init();
#endif
#ifdef CONFIG_BLK_DEV_HD
	directhd_init();
#endif
#ifdef CONFIG_BLK_DEV_XD
	xd_init();
#endif
#ifdef CONFIG_BLK_DEV_FD
	floppy_init();
#else
	outb_p(0xc, 0x3f2);
#endif
#ifdef CONFIG_BLK_DEV_BIOS
	init_bioshd();
#endif	
	return 0;
}
