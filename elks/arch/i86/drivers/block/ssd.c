/*
 * ELKS Solid State Disk /dev/ssd multifunction block device driver
 *      Uses subdriver for particular SSD device
 *      ssd-sd.c - compact flash driver for 8018X
 *      ssd-xms.c - ramdisk driver, use XMS memory
 *      ssd-test.c - test driver, use main memory
 *
 * Rewritten June 2020 Greg Haerr
 * Rewritten to be async I/O capable Aug 2023 Greg Haerr
 * Added XMS support Mar 2025
 */
#include <linuxmt/config.h>
#include <linuxmt/kernel.h>
#include <linuxmt/errno.h>
#include <linuxmt/debug.h>

#define MAJOR_NR    SSD_MAJOR
#include "blk.h"
#include "ssd.h"

/* enable for async callback testing, requires CONFIG_ASYNCIO */
//#define IODELAY     (5*HZ/100)  /* async time delay 5/100 sec = 50msec */

jiff_t ssd_timeout;

sector_t ssd_num_sects;         /* max # sectors on SSD device */
char ssd_initialized;
static int access_count;

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
    if (register_blkdev(MAJOR_NR, DEVICE_NAME, &ssd_fops))
        return;
    blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;

#ifdef CONFIG_FS_XMS_RAMDISK
    xms_init();
    if (!xms_enabled)
        printk("ssd: no XMS\n");
    else printk("ssd: %uK avail\n",
        SETUP_XMS_KBYTES - (xms_alloc_ptr - KBYTES(XMS_START_ADDR)));
#else
    ssd_num_sects = ssddev_init();
    if (ssd_num_sects)
        printk("ssd: %ldK disk\n", ssd_num_sects/2UL);
    else printk("ssd: init error\n");
#endif
}

static int ssd_open(struct inode *inode, struct file *filp)
{
    debug_blk("SSD: open\n");
#ifndef CONFIG_FS_XMS
    if (!ssd_num_sects)
        return -ENODATA;
#endif
    ++access_count;
    inode->i_size = ssd_num_sects << 9;
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

        if (start + req->rq_nr_sectors > ssd_num_sects) {
            printk("ssd: sector %lu+%d beyond max %lu\n", start,
                req->rq_nr_sectors, ssd_num_sects);
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
#if IODELAY
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
#if IODELAY
        ssd_timeout = jiffies + IODELAY;    /* schedule completion callback */
        return;
#else
        ssd_io_complete();                  /* synchronous I/O */
        return;
#endif
    }
}
