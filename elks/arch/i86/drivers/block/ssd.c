/*
 * ELKS Solid State Disk block device driver
 *      Use subdriver for particular SSD device
 *      ssd_test.c - test driver using allocated main memory
 *
 * Rewritten June 2020 Greg Haerr
 * Rewritten to be async I/O capable Aug 2023 Greg Haerr
 */
#include <linuxmt/config.h>
#include <linuxmt/kernel.h>
#include <linuxmt/errno.h>
#include <linuxmt/debug.h>

#define MAJOR_NR    SSD_MAJOR
#include "blk.h"
#include "ssd.h"

#define IODELAY     (5*HZ/100)  /* async time delay 5/100 sec = 50msec */

jiff_t ssd_timeout;

static sector_t NUM_SECTS = 0;  /* max # sectors on SSD device */
static int access_count;
char ssd_initialized;

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

void INITPROC ssd_init(void)
{
    if (register_blkdev(MAJOR_NR, DEVICE_NAME, &ssd_fops) == 0) {
        blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
        NUM_SECTS = ssddev_init();
    }
    if (NUM_SECTS)
        printk("ssd: %ldK disk\n", NUM_SECTS/2UL);
    else printk("ssd: init error\n");
}

static int ssd_open(struct inode *inode, struct file *filp)
{
    debug_blk("SSD: open\n");
    if (!NUM_SECTS)
        return -ENODATA;
    ++access_count;
    inode->i_size = NUM_SECTS << 9;
    return 0;
}

static void ssd_release(struct inode *inode, struct file *filp)
{
    kdev_t dev = inode->i_rdev;

    debug_blk("SSD: release\n");
    if (--access_count == 0) {
        fsync_dev(dev);
        invalidate_inodes(dev);
        invalidate_buffers(dev);
    }
}

/* called by timer interrupt if async operation */
void ssd_io_complete(void)
{
    struct request *req;
    int ret;

    ssd_timeout = 0;        /* stop further callbacks */

#ifdef CHECK_BLOCKIO
    req = CURRENT;
    if (!req) {
        printk("ssd_io_complete: NULL request\n");
        return;
    }
#endif

    for (;;) {
        char *buf;
        int count;
        sector_t start;

        req = CURRENT;
        if (!req)
            return;
        CHECK_REQUEST(req);

        buf = req->rq_buffer;
        start = req->rq_sector;

        if (start + req->rq_nr_sectors > NUM_SECTS) {
            printk("ssd: sector %lu+%d beyond max %lu\n", start,
                req->rq_nr_sectors, NUM_SECTS);
            end_request(0);
            continue;
        }
        for (count = 0; count < req->rq_nr_sectors; count++) {
            if (req->rq_cmd == WRITE) {
                debug_blk("SSD: writing sector %lu\n", start);
                ret = ssddev_write(start, buf, req->rq_seg);
            } else {
                debug_blk("SSD: reading sector %lu\n", start);
                ret = ssddev_read(start, buf, req->rq_seg);
            }
            if (ret != 1)           /* I/O error */
                break;
            start++;
            buf += SD_FIXED_SECTOR_SIZE;
        }
        end_request(count == req->rq_nr_sectors);
#ifdef CONFIG_ASYNCIO
        if (CURRENT) {              /* schedule next completion callback */
            ssd_timeout = jiffies + IODELAY;
        }
        return;                     /* handle only one request per interrupt */
#endif
    }
}

/* called by add_request to start I/O after first request added */
static void do_ssd_request(void)
{
    debug_blk("do_ssd_request\n");
    for (;;) {
        struct request *req = CURRENT;
        if (!req) {
            printk("do_ssd_request: NULL request\n");
            return;
        }
        CHECK_REQUEST(req);

        if (!ssd_initialized) {
            end_request(0);
            return;
        }
#ifdef CONFIG_ASYNCIO
        ssd_timeout = jiffies + IODELAY;    /* schedule completion callback */
        return;
#else
        ssd_io_complete();                  /* synchronous I/O */
        return;
#endif
    }
}
