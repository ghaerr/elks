/*
 * ELKS ATA-CF driver
 *
 * Please note that this driver does not currently support partitions.
 *
 * This driver is largely based on Greg Haerr's SSD driver.
 *
 * Ferry Hendrikx, June 2025
 */

#include <linuxmt/config.h>
#include <linuxmt/kernel.h>
#include <linuxmt/errno.h>
#include <linuxmt/debug.h>
#include <arch/ata.h>
#include "ata.h"
#include "ata-cf.h"
#include "blk.h"


/**********************************************************************
 * device initialisation
 **********************************************************************/

jiff_t ata_cf_timeout;

char ata_cf_initialized;
sector_t ata_cf_num_sects[NUM_DRIVES];     /* max # sectors on SSD device */
static int access_count[NUM_DRIVES];


static int ata_cf_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned int arg);
static int ata_cf_open(struct inode *, struct file *);
static void ata_cf_release(struct inode *, struct file *);

static struct file_operations ata_cf_fops = {
    NULL,                       /* lseek */
    block_read,                 /* read */
    block_write,                /* write */
    NULL,                       /* readdir */
    NULL,                       /* select */
    ata_cf_ioctl,               /* ioctl */
    ata_cf_open,                /* open */
    ata_cf_release              /* release */
};


/**********************************************************************
 * ATA-CF functions
 **********************************************************************/

void INITPROC ata_cf_init(void)
{
    sector_t sectors;
    int i;

    if (register_blkdev(MAJOR_NR, DEVICE_NAME, &ata_cf_fops) == 0)
        blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;

    for (i = 0; i < NUM_DRIVES; i++)
    {
        sectors = ata_init(i);

        if (sectors > 0)
            printk("ata-cf: drive=%d sectors=%ld size=%ldK\n", i, sectors, sectors >> 1);
        else
            printk("ata-cf: drive=%d not present\n", i);

        ata_cf_num_sects[i] = sectors;
    }

    ata_cf_initialized = 1;
}

int ata_cf_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned int arg)
{
    /* not used */
    return -EINVAL;
}

static int ata_cf_open(struct inode *inode, struct file *filp)
{
    int drive = DEVICE_NR(inode->i_rdev);

    debug_blk("ata-cf: open\n");

    if (!ata_cf_num_sects[drive])
        return -ENODATA;

    ++access_count[drive];
    inode->i_size = ata_cf_num_sects[drive] << 9;

    return 0;
}

static void ata_cf_release(struct inode *inode, struct file *filp)
{
    int drive = DEVICE_NR(inode->i_rdev);
    kdev_t dev = inode->i_rdev;

    debug_blk("ata-cf: release\n");
    if (--access_count[drive] == 0) {
        fsync_dev(dev);
        invalidate_inodes(dev);
        invalidate_buffers(dev);
    }
}

/* called by timer interrupt if async operation */
void ata_cf_io_complete(void)
{
    struct request *req;
    int ret;

    ata_cf_timeout = 0;        /* stop further callbacks */

#ifdef CHECK_BLOCKIO
    req = CURRENT;
    if (!req) {
        printk("ata_cf_io_complete: NULL request\n");
        return;
    }
#endif

    for (;;) {
        unsigned char drive;
        char *buf;
        int count;
        sector_t start;

        req = CURRENT;
        if (!req)
            return;
        CHECK_REQUEST(req);

        drive = DEVICE_NR(req->rq_dev);
        buf = req->rq_buffer;
        start = req->rq_sector;

        if (start + req->rq_nr_sectors > ata_cf_num_sects[drive]) {
            printk("ata-cf: sector %lu+%d beyond max %lu\n", start,
                req->rq_nr_sectors, ata_cf_num_sects[drive]);
            end_request(0);
            continue;
        }
        for (count = 0; count < req->rq_nr_sectors; count++) {
            if (req->rq_cmd == WRITE) {
                debug_blk("ata-cf: writing sector %lu\n", start);
                ret = ata_write(drive, start, buf, req->rq_seg);
            } else {
                debug_blk("ata-cf: reading sector %lu\n", start);
                ret = ata_read(drive, start, buf, req->rq_seg);
            }
            if (ret != 1)           /* I/O error */
                break;
            start++;
            buf += ATA_SECTOR_SIZE;
        }
        end_request(count == req->rq_nr_sectors);
#if IODELAY
        if (CURRENT) {              /* schedule next completion callback */
            ata_cf_timeout = jiffies + IODELAY;
        }
        return;                     /* handle only one request per interrupt */
#endif
    }
}

/* called by add_request to start I/O after first request added */
static void do_ata_cf_request(void)
{
    debug_blk("do_ata_cf_request\n");

    for (;;) {
        struct request *req = CURRENT;
        if (!req) {
            printk("do_ata_cf_request: NULL request\n");
            return;
        }
        CHECK_REQUEST(req);

        if (!ata_cf_initialized) {
            end_request(0);
            return;
        }
#if IODELAY
        ata_cf_timeout = jiffies + IODELAY;    /* schedule completion callback */
        return;
#else
        ata_cf_io_complete();               /* synchronous I/O */
        return;
#endif
    }
}
