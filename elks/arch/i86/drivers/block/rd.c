/*
 * ELKS Ramdisk driver
 *
 * rd.c, written by Al Riddoch
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
#include <linuxmt/debug.h>

#define MAJOR_NR RAM_MAJOR
#define RAMDISK
#include "blk.h"

#define MAX_DRIVES	2	/* # ram drives*/
#define MAX_SEGMENTS	8	/* max # seperate allocation segments*/
#define ALLOC_SIZE	4096	/* allocation size in paragaphs*/
#define PARA		16	/* size of paragraph*/
#define SECTOR_SIZE	512

typedef __u16 rd_sector_t;	/* sector number*/

static struct {			/* ramdrive information*/
    int start;			/* starting memory segment*/
    int valid;			/* ramdisk data valid flag*/
    rd_sector_t size;		/* ramdisk size in 512 byte sectors*/
} drive_info[MAX_DRIVES] = {
#ifdef CONFIG_PRELOAD_RAMDISK
    {0, BUSY, 256},
#endif
};

static struct {			/* memory segments, max size ALLOC_SIZE paras (64k)*/
    seg_t seg;			/* actual segment address*/
    segment_s *seg_desc;	/* segment descriptor for heap mgr*/
    int next;			/* next segment*/
    rd_sector_t sectors;	/* segment size in sectors*/
} rd_segment[MAX_SEGMENTS] = {
#ifdef CONFIG_PRELOAD_RAMDISK
    {0x6000, 0,  1, 128,},	/* preloaded ramdisk is 128k at seg 6000:0000*/
    {0x7000, 0, -1, 128,},
#else
    {0,      0, -1, 0,},
    {0,      0, -1, 0,},
#endif
    {0,      0, -1, 0,},
    {0,      0, -1, 0,},
    {0,      0, -1, 0,},
    {0,      0, -1, 0,},
    {0,      0, -1, 0,},
    {0,      0, -1, 0,}
};

static int rd_initialised = 0;

static int rd_open(struct inode *inode, struct file *filp)
{
    int target = DEVICE_NR(inode->i_rdev);

    debug("RD: open ram%d\n", target);
    if (!rd_initialised || target >= MAX_DRIVES)
	return -ENXIO;

    inode->i_size = (long)drive_info[target].size << 9;
    return 0;
}

static void rd_release(struct inode * inode, struct file * filp)
{
    debug("RD: release ram%d\n", DEVICE_NR(inode->i_rdev));
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
		size = size / (SECTOR_SIZE / PARA);
		rd_segment[j].sectors = size;
		drive_info[target].size += rd_segment[j].sectors;
		size = (long) rd_segment[j].sectors * SECTOR_SIZE;
		debug("RD: index %d set to %d sectors, %ld bytes\n",
		       j, rd_segment[j].sectors, size);

		debug("RD: fmemsetw(0, 0x%x, 0, 0x%x)\n",
			rd_segment[j].seg, (word_t) (size >> 1));
		fmemsetw(0, rd_segment[j].seg, 0, (word_t) (size >> 1));

		if (k != -1)
		    rd_segment[k].next = j;	/* set link to next index */
		k = j;
	}
	drive_info[target].valid = 1;
	debug("RD: ramdisk %d created sectors %d index %d bytes %ld\n",
		target, drive_info[target].size, drive_info[target].start,
		(long)drive_info[target].size * SECTOR_SIZE);
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
    byte_t *buff;

    while (1) {
	if (!CURRENT || CURRENT->rq_dev < 0)
	    return;

	INIT_REQUEST;

	if (!CURRENT || CURRENT->rq_sector == (sector_t) -1)
	    return;

	if (!rd_initialised) {
	    end_request(0);
	    return;
	}

	start = (rd_sector_t) CURRENT->rq_sector;
	buff = (byte_t *) CURRENT->rq_buffer;
	target = DEVICE_NR(CURRENT->rq_dev);
	debug("RD: %s dev %d sector %d, ", CURRENT->rq_cmd == READ? "read": "write",
		target, start);
	if (drive_info[target].valid == 0 || start >= drive_info[target].size) {
	    debug("RD: bad request on ram%d, size %d, sector %d\n",
		 target, drive_info[target].size, start);
	    end_request(0);
	    continue;
	}

	/* find appropriate memory segment and sector offset*/
	offset = start;
	index = drive_info[target].start;
	debug("index %d, ", index);
	while (offset > rd_segment[index].sectors) {
	    offset -= rd_segment[index].sectors;
	    index = rd_segment[index].next;
	}
	debug("entry %d, seg %x, offset %d\n", index, rd_segment[index].seg, offset);

	if (CURRENT->rq_cmd == WRITE) {
	    fmemcpyw((byte_t *) (offset * SECTOR_SIZE), rd_segment[index].seg,
		buff, CURRENT->rq_seg, 1024/2);
	} else {
	    fmemcpyw(buff, CURRENT->rq_seg,
		(byte_t *) (offset * SECTOR_SIZE), rd_segment[index].seg, 1024/2);
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
#ifdef BLOAT_FS
    ,NULL,			/* fsync */
    NULL,			/* check_media_change */
    NULL			/* revalidate */
#endif
};

void rd_init(void)
{
#ifndef CONFIG_SMALL_KERNEL
    /* Al's very own ego boost :) */
    printk("rd: ramdisk driver Copyright (C) 1997 Alistair Riddoch\n");
#endif
    if (register_blkdev(MAJOR_NR, DEVICE_NAME, &rd_fops) == 0) {
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
	rd_initialised = 1;
    } else
	printk("rd: unable to register %d\n", MAJOR_NR);
}
