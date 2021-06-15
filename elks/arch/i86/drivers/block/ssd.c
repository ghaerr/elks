/*
 * ELKS Solid State Disk block device driver
 *	Use subdriver for particular SSD device
 *	ssd_test.c - test driver using allocated main memory
 *
 * Rewritten June 2020 Greg Haerr
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
#include "ssd.h"

static sector_t NUM_SECTS = 0;		/* max # sectors on SSD device */

static int ssd_open(struct inode *, struct file *);
static void ssd_release(struct inode *, struct file *);

static struct file_operations ssd_fops = {
    NULL,			/* lseek */
    block_read,			/* read */
    block_write,		/* write */
    NULL,			/* readdir */
    NULL,			/* select */
    ssddev_ioctl,		/* ioctl */
    ssd_open,			/* open */
    ssd_release			/* release */
};

void ssd_init(void)
{
    if (register_blkdev(MAJOR_NR, DEVICE_NAME, &ssd_fops) == 0) {
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
	NUM_SECTS = ssddev_init();
    }
    if (NUM_SECTS)
	printk("ssd: %ldK disk\n", NUM_SECTS/2);
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

static void do_ssd_request(void)
{
    char *buf;
    seg_t seg;
    sector_t start;
    int ret;

    while (1) {
	if (!CURRENT)
	    return;

	INIT_REQUEST;

	if (!CURRENT || CURRENT->rq_sector == (sector_t) -1)
	    return;

	if (!NUM_SECTS) {
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
	    ret = ssddev_write_blk(start, buf, seg);
	} else {
	    debug("SSD: reading sector %lu\n", start);
	    ret = ssddev_read_blk(start, buf, seg);
	}
	if (ret == 2)
	    end_request(1); /* success */
	else end_request(0);
    }
}
