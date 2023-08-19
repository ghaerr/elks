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

#define MAJOR_NR SSD_MAJOR
#define SSDDISK
#include "blk.h"
#include "ssd.h"

#define IODELAY     (5*HZ/100)  /* async time delay 5/100 sec = 50msec */

jiff_t ssd_timeout;

static sector_t NUM_SECTS = 0;  /* max # sectors on SSD device */

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
    debug_blk("SSD: open\n");
    if (!NUM_SECTS)
        return -ENXIO;
    inode->i_size = NUM_SECTS << 9;
    return 0;
}

static void ssd_release(struct inode *inode, struct file *filp)
{
    debug_blk("SSD: release\n");
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
        ramdesc_t seg;
        sector_t start;

        req = CURRENT;
        if (!req)
            return;
        CHECK_REQUEST(req);

#ifdef CHECK_BLOCKIO
        struct buffer_head *bh = req->rq_bh;
        if (req->rq_buffer != buffer_data(bh) || req->rq_seg != buffer_seg(bh)) {
           panic("SSD: ADDR CHANGED req seg:buf %04x:%04x bh seg:buf %04x:%04x\n",
                req->rq_seg, req->rq_buffer, buffer_seg(bh), buffer_data(bh));
        }
        if (req->rq_blocknr != buffer_blocknr(bh)) {
            panic("SSD: BLOCKNR CHANGED req %ld bh %ld\n",
                req->rq_blocknr, buffer_blocknr(bh));
        }
#endif

        buf = req->rq_buffer;
        seg = req->rq_seg;
        start = req->rq_blocknr * (BLOCK_SIZE / SD_FIXED_SECTOR_SIZE);

        /* all ELKS requests are 1K blocks = 2 sectors */
        if (start >= NUM_SECTS-1) { // FIXME move to ll_rw_blk level
            printk("SSD: bad request block %lu cmd %d\n", start/2, req->rq_cmd);
            end_request(0);
            continue;
        }
        if (req->rq_cmd == WRITE) {
            debug_blk("SSD: writing block %lu\n", start/2);
            ret = ssddev_write_blk(start, buf, seg);
        } else {
            debug_blk("SSD: reading block %lu\n", start/2);
            ret = ssddev_read_blk(start, buf, seg);
        }
        if (ret != 2) {
            end_request(0);         /* I/O error */
            continue;
        }
        end_request(1);             /* success */
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

        if (!NUM_SECTS) {
            end_request(0);
            continue;
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
