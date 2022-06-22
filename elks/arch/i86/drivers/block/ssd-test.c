/*
 * ELKS Solid State Disk test subdriver
 *
 * June 2020 Greg Haerr
 *
 * Use ramdisk utility for testing: (block argument = half of NUM_SECTS below)
 *	ramdisk /dev/ssd make 96
 *	mkfs /dev/ssd 96
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
#include <linuxmt/fs.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/errno.h>
#include <linuxmt/debug.h>
#include "ssd.h"

#define NUM_SECTS	192		/* set to max # sectors on SSD device */

static segment_s *ssd_seg = 0;

/* initialize SSD device */
sector_t ssddev_init(void)
{
    return NUM_SECTS;	/* return max # 512-byte sectors on device */
}

int ssddev_ioctl(struct inode *inode, struct file *file,
			unsigned int cmd, unsigned int arg)
{
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
    return -EINVAL;
}

/* write one 1K block (two sectors) to SSD */
int ssddev_write_blk(sector_t start, char *buf, ramdesc_t seg)
{
    unsigned long offset = start << 9;

    xms_fmemcpyw(0, ssd_seg->base + (unsigned int)(offset >> 4), buf, seg, 1024/2);
    return 2;	/* # sectors written */
}

/* read one 1K block (two sectors) from SSD */
int ssddev_read_blk(sector_t start, char *buf, ramdesc_t seg)
{
    unsigned long offset = start << 9;

    xms_fmemcpyw(buf, seg, 0, ssd_seg->base + (unsigned int)(offset >> 4), 1024/2);
    return 2;	/* # sectors read */
}
