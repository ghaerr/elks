/*
 * ELKS Solid State Disk block device driver
 *
 * Rewritten June 2020 Greg Haerr
 *
 * #define TEST to use ramdisk utility for testing:
 *	ramdisk /dev/ssd make 192
 *	mkfs /dev/ssd 192
 *	sync
 *	fsck -lvf /dev/ssd
 *	mount /dev/ssd /mnt
 *	cp /bin/ls /mnt
 *	/mnt/ls
 *	sync
 *	df /dev/ssd
 *	umount /dev/ssd
 *	fsck -lvf /dev/ssd
 */
//#define DEBUG 1
#include <linuxmt/config.h>
#include <linuxmt/rd.h>
#include <linuxmt/major.h>
#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/errno.h>
#include <linuxmt/debug.h>

#define MAJOR_NR SSD_MAJOR
#define SSDDISK
#include "blk.h"

#define TEST 		1		/* test using allocated main memory */

#define NUM_SECTS	192		/* set to max # sectors on SSD device */

static int ssd_initialised = 0;
#if TEST
static segment_s *ssd_seg = 0;
#endif

static int ssd_open(struct inode *, struct file *);
static void ssd_release(struct inode *, struct file *);
static int ssd_ioctl(struct inode *, struct file *, unsigned int, unsigned int);

static struct file_operations ssd_fops = {
    NULL,			/* lseek */
    block_read,			/* read */
    block_write,		/* write */
    NULL,			/* readdir */
    NULL,			/* select */
    ssd_ioctl,			/* ioctl */
    ssd_open,			/* open */
    ssd_release			/* release */
};

void ssd_init(void)
{
    printk("SSD Driver\n");
    if (register_blkdev(MAJOR_NR, DEVICE_NAME, &ssd_fops) == 0) {
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
	ssd_initialised = 1;
	/* add code to initialize SSD */
    }
    else printk("SSD: unable to register %d\n", MAJOR_NR);
}

static int ssd_open(struct inode *inode, struct file *filp)
{
    debug("SSD: open\n");
    if (!ssd_initialised)
	return -ENXIO;
    inode->i_size = (unsigned long)NUM_SECTS << 9;
    return 0;
}

static void ssd_release(struct inode *inode, struct file *filp)
{
    debug("SSD: release\n");
}

static int ssd_ioctl(struct inode *inode, struct file *file,
			unsigned int cmd, unsigned int arg)
{
#if TEST
    unsigned int sector;

    if (!suser())
	return -EPERM;

    switch (cmd) {
    case RDCREATE:
	debug("SSD: ioctl make %d\n", arg); /* size ignored, always NUM_SECTS */
	if (ssd_seg)
	    return -EBUSY;
	ssd_seg = seg_alloc((segext_t)NUM_SECTS << 5, SEG_FLAG_RAMDSK);
	if (!ssd_seg)
	    return -ENOMEM;

	for (sector = 0; sector < NUM_SECTS; sector++) {
	    unsigned long offset = (unsigned long)sector << 9;
	    fmemsetw(0, ssd_seg->base + (unsigned int)(offset >> 4), 0, 256);
	}
	return 0;

    case RDDESTROY:
	debug("SSD: ioctl kill\n");
	if (ssd_seg) {
	    invalidate_inodes(inode->i_rdev);
	    invalidate_buffers(inode->i_rdev);
	    seg_put(ssd_seg);
	    ssd_seg = NULL;
	    return 0;
	}
        break;
    }
#endif
    return -EINVAL;
}

/* write one 1K block (two sectors) to SSD */
static void ssd_write_blk(sector_t start, char *buf, seg_t seg)
{
#if TEST
    unsigned long offset = start << 9;

    fmemcpyw(0, ssd_seg->base + (unsigned int)(offset >> 4), buf, seg, 512);
#else
    /* add code to write to SSD */
#endif
}

/* read one 1K block (two sectors) from SSD */
static void ssd_read_blk(sector_t start, char *buf, seg_t seg)
{
#if TEST
    unsigned long offset = start << 9;

    fmemcpyw(buf, seg, 0, ssd_seg->base + (unsigned int)(offset >> 4), 512);
#else
    /* add code to read from SSD */
#endif
}

static void do_ssd_request(void)
{
    char *buf;
    seg_t seg;
    sector_t start;

    while (1) {
	if (!CURRENT)
	    return;

	INIT_REQUEST;

	if (!CURRENT || CURRENT->rq_sector == (sector_t) -1)
	    return;

	if (!ssd_initialised) {
	    end_request(0);
	    continue;
	}

	buf = CURRENT->rq_buffer;
	seg = CURRENT->rq_seg;
	start = CURRENT->rq_sector;
	if (start >= NUM_SECTS) {
	    debug("SSD: bad request sector %lu\n", start);
	    end_request(0);
	    continue;
	}
	if (CURRENT->rq_cmd == WRITE) {
	    debug("SSD: writing sector %lu\n", start);
	    ssd_write_blk(start, buf, seg);
	} else {
	    debug("SSD: reading sector %lu\n", start);
	    ssd_read_blk(start, buf, seg);
	}
	end_request(1);
    }
}
