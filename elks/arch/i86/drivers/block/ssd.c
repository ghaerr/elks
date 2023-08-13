/*
 * ELKS Solid State Disk block device driver
 *      Use subdriver for particular SSD device
 *      ssd_test.c - test driver using allocated main memory
 *
 * Rewritten June 2020 Greg Haerr
 * Rewritten for async I/O Aug 2023 Greg Haerr
 */
#include <linuxmt/config.h>
#include <linuxmt/debug.h>
#if DEBUG_BLK
#define DEBUG 1
#endif
#include <linuxmt/kernel.h>
#include <linuxmt/errno.h>

#define MAJOR_NR SSD_MAJOR
#define SSDDISK
#include "blk.h"
#include "ssd.h"

static sector_t NUM_SECTS = 0;          /* max # sectors on SSD device */

static int ssd_open(struct inode *, struct file *);
static void ssd_release(struct inode *, struct file *);

static struct file_operations ssd_fops = {
    NULL,                       /* lseek */
    block_read,                 /* read */
    block_write,                /* write */
    NULL,                       /* readdir */
    NULL,                       /* select */
    ssddev_ioctl,               /* ioctl */
    ssd_open,                   /* open */
    ssd_release                 /* release */
};

void ssd_init(void)
{
    if (register_blkdev(MAJOR_NR, DEVICE_NAME, &ssd_fops) == 0) {
        blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
        NUM_SECTS = ssddev_init();
    }
    if (NUM_SECTS)
        printk("ssd: %ldK disk\n", NUM_SECTS/2UL);
    else printk("ssd: initialization error\n");
}

static int ssd_open(struct inode *inode, struct file *filp)
{
    debug("SSD: open\n");
    if (!NUM_SECTS)
        return -ENXIO;
    inode->i_size = NUM_SECTS << 9;
    return 0;
}

static void ssd_release(struct inode *inode, struct file *filp)
{
    debug("SSD: release\n");
}

#define IODELAY     (5*HZ/100)  /* 5/100 sec = 50msec */

jiff_t ssd_timeout;

/* called by timer interrupt */
void ssd_io_complete(void)
{
    struct request *req;
    int ret;

    ssd_timeout = 0;        /* stop further callbacks */

#ifdef CHECK_BLOCKIO
    req = CURRENT;
    if (!req || req->rq_dev == -1U || req->rq_status != RQ_ACTIVE) {
        printk("ssd_io_complete: NULL request %04x\n", req);
        return;
    }
#endif

    for (;;) {
        char *buf;
        ramdesc_t seg;
        sector_t start;

        req = CURRENT;
        if (!req)
            return;

#ifdef CHECK_BLOCKIO
        if (req->rq_dev == -1U || req->rq_status != RQ_ACTIVE) {
            printk("ssd_io_complete: bad request\n");
            end_request(0);
            continue;
        }

        struct buffer_head *bh = req->rq_bh;
        if (req->rq_buffer != buffer_data(bh) || req->rq_seg != buffer_seg(bh)) {
           printk("SSD: ***ADDR CHANGED*** req seg:buf %04x:%04x bh seg:buf %04x:%04x\n",
                req->rq_seg, req->rq_buffer, buffer_seg(bh), buffer_data(bh));
        }
        if (req->rq_blocknr != buffer_blocknr(bh)) {
            printk("SSD: ***BLOCKNR CHANGED*** req %ld bh %ld\n",
                req->rq_blocknr, buffer_blocknr(bh));
        }
#endif

        buf = req->rq_buffer;
        seg = req->rq_seg;
        start = req->rq_blocknr * (BLOCK_SIZE / SD_FIXED_SECTOR_SIZE);

        /* all ELKS requests are 1K blocks = 2 sectors */
        if (start >= NUM_SECTS-1) {
            debug("SSD: bad request block %lu\n", start/2);
            end_request(0);
            continue;
        }
        if (req->rq_cmd == WRITE) {
            debug("SSD: writing block %lu\n", start/2);
            ret = ssddev_write_blk(start, buf, seg);
        } else {
            debug("SSD: reading block %lu\n", start/2);
            ret = ssddev_read_blk(start, buf, seg);
        }
        if (ret != 2) {
            end_request(0);         /* I/O error */
            continue;
        }
        end_request(1);             /* success */
        if (CURRENT) {
            ssd_timeout = jiffies + IODELAY;    /* schedule next completion callback */
        }
    }
}

/* called by add_request to start I/O after first request added */
static void do_ssd_request(void)
{
    debug_blk("do_ssd_request\n");
    for (;;) {
        struct request *req = CURRENT;

#ifdef CHECK_BLOCKIO
        if (!req || req->rq_dev == -1U) {
            printk("do_ssd_request: no requests\n");
            return;
        }
#endif
        INIT_REQUEST(req);

        if (!NUM_SECTS) {
            end_request(0);
            continue;
        }
        ssd_timeout = jiffies + IODELAY;    /* schedule completion callback */
        return;
    }
}
