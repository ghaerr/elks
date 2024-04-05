/*
 * ELKS Ramdisk driver
 *
 * rd.c, written by Al Riddoch
 * ramdisk driver Copyright (C) 1997 Alistair Riddoch
 * modified: July 4th, 5th 1999, Blaz Antonic
 * - allow variable ramdisk size (in 4096 blocks)
 * - allow ramdisks larger than 64 KB (which is segment limit)
 * modified: September 20th 1999, Blaz Antonic
 * - fix variable type mismatches throughout the code
 * modified: October 2nd 1999, Blaz Antonic
 * - further bugfixes, redesigned to work with 1024 byte allocation units
 * modified: October 4th 1999, Blaz Antonic
 * - final changes before i put this out to public :)
 * Debugged and rewritten Oct 2020 by Greg Haerr
 */

#include <linuxmt/config.h>
#include <linuxmt/rd.h>
#include <linuxmt/major.h>
#include <linuxmt/kernel.h>
#include <linuxmt/errno.h>
#include <linuxmt/mm.h>
#include <linuxmt/fs.h>
#include <linuxmt/debug.h>

#define MAJOR_NR    RAM_MAJOR
#include "blk.h"

#define MAX_DRIVES	2	/* # ram drives*/
#define MAX_SEGMENTS	8	/* max # seperate allocation segments*/
#define ALLOC_SIZE	4096	/* allocation size in paragaphs*/
#define PARA		16	/* size of paragraph*/

/* if sector size not 512, must implement IOCTL_BLK_GET_SECTOR_SIZE */
#define RD_SECTOR_SIZE	512
typedef __u16 rd_sector_t;	/* sector number*/

static struct {			/* ramdrive information*/
    int start;			/* starting memory segment*/
    int valid;			/* ramdisk data valid flag*/
    rd_sector_t size;		/* ramdisk size in 512 byte sectors*/
} drive_info[MAX_DRIVES] = {
#if CONFIG_RAMDISK_SEGMENT
    {0, 1, CONFIG_RAMDISK_SECTORS}
#endif
};

static struct {			/* memory segments, max size ALLOC_SIZE paras (64k)*/
    seg_t seg;			/* actual segment address*/
    segment_s *seg_desc;	/* segment descriptor for heap mgr*/
    int next;			/* next segment*/
    rd_sector_t sectors;	/* segment size in sectors*/
} rd_segment[MAX_SEGMENTS] = {
#if CONFIG_RAMDISK_SEGMENT	/* preloaded ramdisk*/
    {CONFIG_RAMDISK_SEGMENT, 0,  -1, CONFIG_RAMDISK_SECTORS},
#else
    {0,      0, -1, 0,},
#endif
    {0,      0, -1, 0,},
    {0,      0, -1, 0,},
    {0,      0, -1, 0,},
    {0,      0, -1, 0,},
    {0,      0, -1, 0,},
    {0,      0, -1, 0,},
    {0,      0, -1, 0,}
};

static int rd_initialised = 0;
static int access_count[MAX_DRIVES];

static int rd_open(struct inode *inode, struct file *filp)
{
    int target = DEVICE_NR(inode->i_rdev);

    debug("RD: open /dev/rd%d\n", target);
    if (!rd_initialised || target >= MAX_DRIVES
#if CONFIG_RAMDISK_SEGMENT
        || !drive_info[target].valid
#endif
                                                )
        return -ENXIO;

    ++access_count[target];
    inode->i_size = (long)drive_info[target].size << 9;
    return 0;
}

static void rd_release(struct inode * inode, struct file * filp)
{
    kdev_t dev = inode->i_rdev;
    int target = DEVICE_NR(dev);

    debug("RD: release rd%d\n", target);
    if (--access_count[target] == 0) {
        fsync_dev(dev);
        invalidate_inodes(dev);
        invalidate_buffers(dev);
    }
}

static int find_free_seg(void)
{
    int i;

    for (i = 0; i < MAX_SEGMENTS; i++) {
	debug("RD: rd_segment[%d].sectors = %d\n", i, rd_segment[i].sectors);
	if (rd_segment[i].sectors == 0)
	    return i;
    }
    return -1;
}

static int dealloc(int target)
{
    int i, j;

    i = drive_info[target].start;
    debug("RD: dealloc target %d, index %d, size %d sectors\n",
	   target, i, drive_info[target].size);
    while (i != -1 && rd_segment[i].sectors != 0) {
	j = i;
	debug("RD: dealloc purging rd_segment[%d].seg = 0x%x, next index %d, size = %d\n",
	     j, rd_segment[j].seg, rd_segment[j].next, rd_segment[j].sectors);
	if (rd_segment[j].seg_desc)
		seg_put(rd_segment[j].seg_desc);
	rd_segment[j].seg_desc = 0;
	rd_segment[j].seg = 0;
	rd_segment[j].sectors = 0;
	i = rd_segment[j].next;
	rd_segment[j].next = -1;
	debug("RD: dealloc status rd_segment[%d].seg = 0x%x, next index %d, size = %d\n",
	     j, rd_segment[j].seg, rd_segment[j].next, rd_segment[j].sectors);
    }
    drive_info[target].valid = 0;
    drive_info[target].size = 0;
    return 0;
}

static int rd_ioctl(register struct inode *inode, struct file *file,
		    unsigned int cmd, unsigned int arg)
{
    unsigned int i;
    int j, k, target = DEVICE_NR(inode->i_rdev);
    unsigned long size;

    if (!suser())
	return -EPERM;

    debug("RD: ioctl %d %s\n", target, (cmd ? "kill" : "make"));
    switch (cmd) {
    case RDCREATE:
	if (drive_info[target].valid)
	    return -EBUSY;

	drive_info[target].size = 0;
	k = -1;
	for (i = 0; i <= (arg - 1) / ((ALLOC_SIZE / 1024) * PARA); i++) {
		j = find_free_seg();	/* find free place in queue */
		debug("RD: find_free_seg = %d\n", j);
		if (j == -1) {
		    dealloc(target);
		    return -ENOMEM;
		}
		if (i == 0)
		    drive_info[target].start = j;

		if (i == (arg / ((ALLOC_SIZE / 1024) * PARA)))
		    /* size in 16 byte pages = (arg % 64) * 64 */
		    size = (arg % ((ALLOC_SIZE / 1024) * PARA)) *
				  ((ALLOC_SIZE / 1024) * PARA);
		else
		    size = ALLOC_SIZE;

		/* allocate memory */
		rd_segment[j].seg_desc = seg_alloc ((segext_t)size, SEG_FLAG_RAMDSK);
		if (rd_segment[j].seg_desc == 0) {
		    dealloc(target);
		    return -ENOMEM;
		}
		rd_segment[j].seg = rd_segment[j].seg_desc->base;
		debug("RD: pass: %d, allocated %d pages, rd_segment[%d].seg = 0x%x\n",
		       i, (int) size, j, rd_segment[j].seg);

		/* recalculate size to reflect size in sectors, not pages */
		size = size / (RD_SECTOR_SIZE / PARA);
		rd_segment[j].sectors = size;
		drive_info[target].size += rd_segment[j].sectors;
		size = (long) rd_segment[j].sectors * RD_SECTOR_SIZE;
		debug("RD: index %d set to %d sectors, %ld bytes\n",
		       j, rd_segment[j].sectors, size);

		debug("RD: fmemsetw(0, 0x%x, 0, 0x%x)\n",
			rd_segment[j].seg, (size_t) (size >> 1));
		fmemsetw(0, rd_segment[j].seg, 0, (size_t) (size >> 1));

		if (k != -1)
		    rd_segment[k].next = j;	/* set link to next index */
		k = j;
	}
	drive_info[target].valid = 1;
	debug("RD: ramdisk %d created sectors %d index %d bytes %ld\n",
		target, drive_info[target].size, drive_info[target].start,
		(long)drive_info[target].size * RD_SECTOR_SIZE);
	return 0;

    case RDDESTROY:
	if (drive_info[target].valid) {
	    invalidate_inodes(inode->i_rdev);
	    invalidate_buffers(inode->i_rdev);
	    dealloc(target);
	    return 0;
	}
    }
    return -EINVAL;
}

/* low level block request entry point*/
static void do_rd_request(void)
{
    rd_sector_t start;		/* requested start sector*/
    rd_sector_t offset;		/* sector offset in memory segment*/
    int index;
    int target;
    int count;
    byte_t *buf;

    while (1) {
	struct request *req = CURRENT;
	if (!req)
	    return;
	CHECK_REQUEST(req);

	if (!rd_initialised) {
	    end_request(0);
	    return;
	}

	start = (rd_sector_t) req->rq_sector;
	buf = (byte_t *) req->rq_buffer;
	target = DEVICE_NR(req->rq_dev);
	debug("RD: %s dev %d sector %d, ", req->rq_cmd == READ? "read": "write",
		target, start);
	if (drive_info[target].valid == 0 ||
                start + req->rq_nr_sectors > drive_info[target].size) {
	    debug("rd%d: sector %d beyond max %d\n",
		 target, start, drive_info[target].size);
	    end_request(0);
	    continue;
	}

        for (count = 0; count < req->rq_nr_sectors; count++) {
            /* find appropriate memory segment and sector offset*/
            offset = start;
            index = drive_info[target].start;
            debug("index %d, ", index);
            while (offset > rd_segment[index].sectors) {
                offset -= rd_segment[index].sectors;
                index = rd_segment[index].next;
            }
            debug("entry %d, seg %x, offset %d\n", index, rd_segment[index].seg, offset);

            if (req->rq_cmd == WRITE) {
                xms_fmemcpyw((char *) (offset * RD_SECTOR_SIZE), rd_segment[index].seg,
                    buf, req->rq_seg, RD_SECTOR_SIZE/2);
            } else {
                xms_fmemcpyw(buf, req->rq_seg, (byte_t *) (offset * RD_SECTOR_SIZE),
                    rd_segment[index].seg, RD_SECTOR_SIZE/2);
            }
            start++;
            buf += RD_SECTOR_SIZE;;
        }
        end_request(1);
    }
}

static struct file_operations rd_fops = {
    NULL,			/* lseek */
    block_read,			/* read */
    block_write,		/* write */
    NULL,			/* readdir */
    NULL,			/* select */
    rd_ioctl,			/* ioctl */
    rd_open,			/* open */
    rd_release			/* release */
};

void INITPROC rd_init(void)
{
    if (register_blkdev(MAJOR_NR, DEVICE_NAME, &rd_fops) == 0) {
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
	rd_initialised = 1;

#if CONFIG_RAMDISK_SEGMENT
	printk("rd: %dK ramdisk at %x:0000\n",
	    drive_info[0].size >> 1, rd_segment[0].seg);

#if (CONFIG_RAMDISK_SECTORS > 128)
	/* build segment array for preloaded ramdisks > 64k*/
	int i = 0;
	seg_t seg = rd_segment[0].seg;
	rd_sector_t sectors = rd_segment[0].sectors;
	while (sectors > 128 && i < MAX_SEGMENTS-1) {
	    rd_segment[i].sectors = 128;	/* 64k*/
	    rd_segment[i].next = i + 1;
	    i++;
	    rd_segment[i].seg = (seg += 0x1000);/* 64k in paragraphs*/
	    rd_segment[i].sectors = (sectors -= 128);
	    rd_segment[i].next = -1;
	}
#endif
#if DEBUG
	for (int i=0; i < MAX_SEGMENTS; i++)
		printk("%d: seg %x next %d sectors %d\n",
			i, rd_segment[i].seg, rd_segment[i].next, rd_segment[i].sectors);
#endif
#endif /* CONFIG_RAMDISK_SEGMENT*/
    } else
	printk("rd: init error\n");
}
