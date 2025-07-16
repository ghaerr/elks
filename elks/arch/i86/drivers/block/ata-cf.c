/**********************************************************************
 * ELKS ATA-CF driver
 *
 * Please note that this driver does not currently support partitions.
 *
 * This driver is largely based on Greg Haerr's SSD driver.
 *
 * Ferry Hendrikx, June 2025
 * Greg Haerr, July 2025 Add partition handling
 **********************************************************************/

#include <linuxmt/config.h>
#include <linuxmt/kernel.h>
#include <linuxmt/errno.h>
#include <linuxmt/debug.h>
#include <arch/ata.h>

#define MAJOR_NR        ATHD_MAJOR
#include "blk.h"

/* the following must match with BIOSHD /dev minor numbering scheme*/
#define NUM_MINOR       8       /* max minor devices per drive*/
#define MINOR_SHIFT     3       /* =log2(NUM_MINOR) shift to get drive num*/

#define MAX_DRIVES      2       /* <=256/NUM_MINOR*/
#define NUM_DRIVES      2

static int access_count[NUM_DRIVES];
static struct drive_infot ata_drive_info[NUM_DRIVES];   /* operating drive info */
static struct hd_struct hd[NUM_DRIVES << MINOR_SHIFT];  /* partitions start, size*/

static struct gendisk ata_gendisk = {
    MAJOR_NR,                   /* Major number */
    "cf",                       /* Major name */
    MINOR_SHIFT,                /* Bits to shift to get real from partition */
    1 << MINOR_SHIFT,           /* maximum number of partitions per drive */
    NUM_DRIVES,                 /* maximum number of drives */
    hd,                         /* hd struct */
    0,                          /* hd drives found */
    ata_drive_info              /* fd/hd drive CHS and type */
};

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

struct gendisk * INITPROC ata_cf_init(void)
{
    int i;

    // register device

    if (register_blkdev(MAJOR_NR, DEVICE_NAME, &ata_cf_fops))
        return NULL;
    blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;

    ata_reset();

    // ATA drive detect

    for (i = 0; i < NUM_DRIVES; i++) {
        if (ata_init(i, &ata_drive_info[i]))
            ata_gendisk.nr_hd++;
    }

    return &ata_gendisk;
}

int ata_cf_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned int arg)
{
    // FIXME share with bioshd for fdisk geometry

    return -EINVAL;
}

static int ata_cf_open(struct inode *inode, struct file *filp)
{
    int drive = DEVICE_NR(inode->i_rdev);
    struct hd_struct *hdp = &hd[MINOR(inode->i_rdev)];

    debug_blk("cf%d: open\n", drive);

    if (drive >= NUM_DRIVES)
        return -ENXIO;

#if NOTYET
    // reidentify device if its not already in use
    if (access_count[drive] == 0) {
        ata_init(drive, &ata_drive_info[drive]);
        init_partitions(&ata_gendisk);  // INITPROC can't be called after init!
    }
#endif

    if (hdp->start_sect == -1U)
        return -ENXIO;

    ++access_count[drive];
    inode->i_size = hdp->nr_sects * ata_drive_info[drive].sector_size;

    return 0;
}

static void ata_cf_release(struct inode *inode, struct file *filp)
{
    int drive = DEVICE_NR(inode->i_rdev);
    kdev_t dev = inode->i_rdev;

    debug_blk("cf%d: release\n", drive);

    if (--access_count[drive] == 0) {
        fsync_dev(dev);
        invalidate_inodes(dev);
        invalidate_buffers(dev);
    }
}

/* called by add_request to start I/O after first request added */
static void do_ata_cf_request(void)
{
    struct request *req;
    int ret;

#ifdef CHECK_BLOCKIO
    req = CURRENT;
    if (!req) {
        debug_blk("do_ata_cf_request: NULL request\n");
        return;
    }
#endif

    for (;;) {
        int drive;
        unsigned short minor;
        char *buf;
        int count;
        sector_t start;

        req = CURRENT;
        if (!req)
            return;
        CHECK_REQUEST(req);

        minor = MINOR(req->rq_dev);
        drive = minor >> MINOR_SHIFT;
        buf = req->rq_buffer;
        start = req->rq_sector;

        if (hd[minor].start_sect == -1U || start + req->rq_nr_sectors > hd[minor].nr_sects) {
              printk("cf%d: sector %ld access beyond partition (%ld,%ld)\n",
                drive, start, hd[minor].start_sect, hd[minor].nr_sects);
            end_request(0);
            continue;
        }
        start += hd[minor].start_sect;

        for (count = 0; count < req->rq_nr_sectors; count++) {
            if (req->rq_cmd == WRITE) {
                debug_blk("cf%d: writing sector %lu\n", drive, start);
                ret = ata_write(drive, start, buf, req->rq_seg);
            } else {
                debug_blk("cf%d: reading sector %lu\n", drive, start);
                ret = ata_read(drive, start, buf, req->rq_seg);
            }
            if (ret != 0) {         /* I/O error */
                printk("cf%d: I/O error %d cmd %d\n", drive, ret, req->rq_cmd);
                break;
            }
            start++;
            buf += ATA_SECTOR_SIZE;
        }
        end_request(count == req->rq_nr_sectors);
        return;                     /* synchronous I/O */
    }
}
