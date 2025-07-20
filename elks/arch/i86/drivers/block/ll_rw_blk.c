/*
 *  linux/drivers/block/ll_rw_blk.c
 *
 * Copyright (C) 1991, 1992 Linus Torvalds
 * Copyright (C) 1994,      Karl Keyte: Added support for disk statistics
 */

/*
 * This handles all read/write requests to block devices
 */

#include <linuxmt/config.h>
#include <linuxmt/limits.h>
#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/fs.h>
#include <linuxmt/errno.h>
#include <linuxmt/string.h>
#include <linuxmt/init.h>
#include <linuxmt/mm.h>
#include <linuxmt/ioctl.h>
#include <linuxmt/debug.h>

#include <arch/system.h>
#include <arch/io.h>
#include <arch/irq.h>

#include "blk.h"

/*
 * The request-struct contains all necessary data
 * to load a number of sectors into memory
 *
 * NR_REQUEST is the number of entries in the request-queue.
 * NOTE that writes may use only the low 2/3 of these: reads
 * take precedence.
 */
static struct request all_requests[NR_REQUEST];

/* current request and function pointer for each block device handler */
struct blk_dev_struct blk_dev[MAX_BLKDEV];      /* initialized by blk_dev_init() */

/* return hardware sector size for passed device */
int get_sector_size(kdev_t dev)
{
    struct file_operations *fops;
    int size;

    /* get disk sector size using block device ioctl */
    fops = get_blkfops(MAJOR(dev));
    if (!fops || !fops->ioctl ||
        (size = fops->ioctl(NULL, NULL, IOCTL_BLK_GET_SECTOR_SIZE, dev)) <= 0)
            size = 512;
    return size;
}

/*
 * look for a free request in the first N entries.
 * NOTE: interrupts must be disabled on the way in, and will still
 *       be disabled on the way out.
 */
static struct request *get_request(int n, kdev_t dev)
{
    static struct request *prev_found = NULL;
    static struct request *prev_limit = NULL;
    struct request *req;
    struct request *limit;

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
        if (req == prev_found) {
#ifdef CONFIG_ASYNCIO
            return NULL;
#else
            panic("get_request");   /* no inactive requests */
#endif
        }
    }
    prev_found = req;
    req->rq_status = RQ_ACTIVE;
    req->rq_dev = dev;
    return req;
}

#ifdef CONFIG_ASYNCIO
struct wait_queue wait_for_request;

/*
 * wait until a free request in the first N entries is available.
 */
static struct request *__get_request_wait(int n, kdev_t dev)
{
    struct request *req;

    debug_blk("Waiting for request...\n");
    wait_set(&wait_for_request);
    for (;;) {
        current->state = TASK_UNINTERRUPTIBLE;
        clr_irq();
        req = get_request(n, dev);
        set_irq();
        if (req)
            break;
        schedule();
    }
    wait_clear(&wait_for_request);
    current->state = TASK_RUNNING;
    return req;
}
#endif

static struct request *get_request_wait(int n, kdev_t dev)
{
    struct request *req;

    clr_irq();
    req = get_request(n, dev);
    set_irq();
    if (req) return req;
#ifdef CONFIG_ASYNCIO
    /* Try again blocking if no request available */
    return __get_request_wait(n, dev);
#else
    panic("no reqs");
    return NULL;
#endif
}

/*
 * add-request adds a request to the linked list.
 * It disables interrupts so that it can muck with the
 * request-lists in peace.
 */
static void add_request(struct blk_dev_struct *dev, struct request *req)
{
    struct request *tmp;

    clr_irq();
    mark_buffer_clean(req->rq_bh);
    if (!(tmp = dev->current_request)) {
        dev->current_request = req;
        set_irq();
        (dev->request_fn) ();
    }
    else {
#ifdef CONFIG_ASYNCIO
        for (; tmp->rq_next; tmp = tmp->rq_next) {
            if ((IN_ORDER(tmp, req) ||
                !IN_ORDER(tmp, tmp->rq_next)) && IN_ORDER(req, tmp->rq_next))
                break;
        }
        req->rq_next = tmp->rq_next;
        tmp->rq_next = req;
        set_irq();
#if DEBUG_CACHE
        if (debug_level) {
            int n = 0;
            for (tmp = dev->current_request; tmp->rq_next; tmp = tmp->rq_next)
                n++;
            if (n > 1) printk("REQS %d ", n);
        }
#endif
#else
        panic("add_request");   /* non-empty request queue */
#endif
    }
}

static void make_request(unsigned short major, int rw, struct buffer_head *bh)
{
    struct request *req;
    int max_req;

    debug("BLK %lu %s %lx:%x\n", buffer_blocknr(bh), rw==READ? "read": "write",
        (unsigned long)buffer_seg(bh), buffer_data(bh));

    /* Uhhuh.. Nasty dead-lock possible here.. */
    if (EBH(bh)->b_locked)
        return;
    /* Maybe the above fixes it, and maybe it doesn't boot. Life is interesting */
    lock_buffer(bh);

    max_req = NR_REQUEST;       /* reads take precedence */
    switch (rw) {
    case READ:
        break;

    case WRITE:
#if NR_REQUEST != 1             /* protect max_req from being 0 below */
        /* We don't allow the write-requests to fill up the
         * queue completely:  we want some room for reads,
         * as they take precedence. The last third of the
         * requests are only for reads.
         */
        max_req = (NR_REQUEST * 2) / 3;
#endif
#ifdef CHECK_BLOCKIO
        if (!EBH(bh)->b_dirty) {
            printk("make_request: block %ld not dirty\n", EBH(bh)->b_blocknr);
            unlock_buffer(bh);
            return;
        }
#endif
        break;

    default:
        panic("make_request");
        unlock_buffer(bh);
        return;
    }

    /* find an unused request. */
    req = get_request_wait(max_req, buffer_dev(bh));

    /* fill up the request-info, and add it to the queue */
    req->rq_cmd = rw;
    req->rq_nr_sectors = BLOCK_SIZE / get_sector_size(req->rq_dev);
    req->rq_sector = buffer_blocknr(bh) * req->rq_nr_sectors;
    req->rq_seg = buffer_seg(bh);
    req->rq_buffer = buffer_data(bh);
    req->rq_bh = bh;
    req->rq_errors = 0;
    req->rq_next = NULL;
    add_request(&blk_dev[major], req);
}

/* Read/write a single buffer from a block device */
void ll_rw_blk(int rw, struct buffer_head *bh)
{
    struct blk_dev_struct *dev;
    unsigned int major;

    dev = NULL;
    if ((major = MAJOR(buffer_dev(bh))) < MAX_BLKDEV)
        dev = blk_dev + major;
    if (!dev || !dev->request_fn)
        panic("bad blkdev %D", buffer_dev(bh));
    make_request(major, rw, bh);
}

#ifdef MULTI_BH
/*
 * "plug" the device if there are no outstanding requests: this will
 * force the transfer to start only after we have put all the requests
 * on the list.
 */
static void plug_device(struct blk_dev_struct *dev, struct request *plug)
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
    unsigned int major;
    int i;

    /* Make sure the first block contains something reasonable */
    while (!*bh) {
        bh++;
        if (--nr <= 0)
            return;
    }
    dev = NULL;
    if ((major = MAJOR(buffer_dev(bh[0])) < MAX_BLKDEV)
        dev = blk_dev + major;
    if (!dev || !dev->request_fn) {
        printk("ll_rw_block: Trying to read nonexistent block-device %s (%lu)\n",
             kdevname(buffer_dev(bh[0])), buffer_blocknr(bh[0]));
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
            mark_buffer_clean(bh[i]);
            mark_buffer_uptodate(bh[i], 0);
        }
}
#endif /* MULTI_BH */

void INITPROC blk_dev_init(void)
{
#if NOTNEEDED
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
#endif

#ifdef CONFIG_ROMFS_FS
    romflash_init();
#endif

#ifdef CONFIG_BLK_DEV_RAM
    rd_init();          /* RAMDISK block device*/
#endif

#ifdef CONFIG_BLK_DEV_HD
    struct gendisk *hddisk = directhd_init();
#endif

#ifdef CONFIG_BLK_DEV_FD
    floppy_init();      /* direct floppy, init before SSD for possible XMS track cache */
#endif

#if defined(CONFIG_BLK_DEV_SSD_TEST) || defined(CONFIG_BLK_DEV_SSD_SD8018X) || \
    defined(CONFIG_FS_XMS_RAMDISK)
    ssd_init();         /* SSD block device*/
#endif

#if defined(CONFIG_BLK_DEV_BFD) || defined(CONFIG_BLK_DEV_BHD)
    struct gendisk *biosdisk = bioshd_init();
#endif

#ifdef CONFIG_BLK_DEV_ATA_CF
    struct gendisk *atadisk = ata_cf_init();
#endif

#ifdef CONFIG_BLK_DEV_BHD
    if (biosdisk)
        init_partitions(biosdisk);
#endif

#ifdef CONFIG_BLK_DEV_ATA_CF
    if (atadisk)
        init_partitions(atadisk);
#endif
}
