/*
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
 */

#include <linuxmt/config.h>
#include <linuxmt/rd.h>
#include <linuxmt/major.h>
#include <linuxmt/kernel.h>
#include <linuxmt/debug.h>
#include <linuxmt/errno.h>

#ifdef CONFIG_BLK_DEV_RAM

#define MAJOR_NR RAM_MAJOR

#define RAMDISK
#include "blk.h"

#define SECTOR_SIZE 512
#define SEG_SIZE 4096 /* # of 16 B pages */
#define P_SIZE 16 /* 16 B pages */
#define DIVISOR 32 /* SECTOR_SIZE / P_SIZE */
#define MAX_ENTRIES 8

#ifndef DEBUG
#endif

typedef __u16 rd_sector_t;

static int rd_initialised = 0;
static struct rd_infot
{
	unsigned int index;
	unsigned int flags;
	rd_sector_t size; /* ramdisk size in 512 B blocks */
} rd_info[MAX_ENTRIES] = 
{
	{0,0,0},
	{0,0,0},
	{0,0,0},
	{0,0,0},
	{0,0,0},
	{0,0,0},
	{0,0,0},
	{0,0,0}
};

static struct rd_segmentt
{
	seg_t segment;
	unsigned int next;
	rd_sector_t seg_size; /* segment size in 512 byte blocks */
} rd_segment[MAX_ENTRIES] = /* max 640 KB will be used for RAM disk(s) */
{
	{0,MAX_ENTRIES+1,0,},
	{0,MAX_ENTRIES+1,0,},
	{0,MAX_ENTRIES+1,0,},
	{0,MAX_ENTRIES+1,0,},
	{0,MAX_ENTRIES+1,0,},
	{0,MAX_ENTRIES+1,0,},
	{0,MAX_ENTRIES+1,0,},
	{0,MAX_ENTRIES+1,0,}
};

static int rd_open(inode, filp);
static int rd_release(inode, filp);
static int rd_ioctl(inode, file, cmd, arg);

void rd_load() {}

static struct file_operations rd_fops =
{
	NULL,			/* lseek */
	block_read,		/* read */
	block_write,		/* write */
	NULL,			/* readdir */
	NULL,			/* select */
	rd_ioctl,		/* ioctl */
	rd_open,		/* open */
	rd_release,		/* release */
#ifdef BLOAT_FS
	NULL,			/* fsync */
	NULL,			/* check_media_change */
	NULL,			/* revalidate */
#endif
};

void rd_init()
{
	int i;

	/* Al's very own ego boost :) */
	printk("rd driver Copyright (C) 1997 Alistair Riddoch\n");
	if ((i = register_blkdev(MAJOR_NR, DEVICE_NAME, &rd_fops)) == 0) {
		blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
		/* blksize_size[MAJOR_NR] = 1024; */
		/* read_ahead[MAJOR_NR] = 2; */
		rd_initialised = 1;
	} else {
		printk("rd: unable to register\n");
	}
}
	
static int rd_open(inode, filp)
struct inode *inode;
struct file *filp;
{
	int target;

	target = DEVICE_NR(inode->i_rdev);
	printd_rd1("RD_OPEN %d\n",target);
	if (rd_initialised == 0)
		return (-ENXIO);
/*	if (rd_info[target].flags && RD_BUSY)
		return (-EBUSY); */
	return 0;
}

static int rd_release(inode, filp)
struct inode *inode;
struct file *filp;
{
	printd_rd("RD_RELEASE \n");
/*	rd_info[DEVICE_NR(inode->i_rdev)].flags = 0; */ /* ioctl() takes care of it */
	return 0;
}

int find_free_seg()
{
	unsigned int i;
	for (i = 0; i < MAX_ENTRIES; i++) {
#ifdef DEBUG
		printk("find_free_seg(): rd_segment[%d].seg_size = %d\n", i, rd_segment[i].seg_size);
#endif
		if (rd_segment[i].seg_size == 0)
			return i;
	}
	return -1;
}

int rd_dealloc(target)
int target;
{
	unsigned int i, j;
#ifdef DEBUG
	int a = 0;
#endif
	i = rd_info[target].index;
#ifdef DEBUG
	printk("rd_dealloc: target = %d, first index = %d, size = %d blocks\n", target, i, rd_info[target].size);
#endif
	while ((rd_segment[i].seg_size != 0) && (i != MAX_ENTRIES + 1)) {
		j = i;
#ifdef DEBUG
		printk("rd_dealloc: pass: %d, purging rd_segment[%d].segment = 0x%x, next index %d, size = %d\n", a, j, rd_segment[j].segment, rd_segment[j].next, rd_segment[j].seg_size);
#endif
		mm_free(rd_segment[j].segment);
		rd_segment[j].segment = 0;
		rd_segment[j].seg_size = 0;
		i = rd_segment[j].next;
		rd_segment[j].next = MAX_ENTRIES + 1;
#ifdef DEBUG
		printk("rd_dealloc: pass: %d, status rd_segment[%d].segment = 0x%x, next index %d, size = %d\n", a, j, rd_segment[j].segment, rd_segment[j].next, rd_segment[j].seg_size);
		a++;
#endif
	}
	rd_info[target].flags = 0;
	
	return 0;
}

static int rd_ioctl(inode, file, cmd, arg)
register struct inode *inode;
struct file *file;
unsigned int cmd;
unsigned int arg; /* size in 1024 byte blocks */
{
	int target = DEVICE_NR(inode->i_rdev);
	unsigned int i;
	int j, k;
	unsigned long size;

	if (!suser())
		return -EPERM;
	printd_rd2("RD_IOCTL %d %s\n", target, (cmd ? "kill" : "make"));
	switch(cmd) {
		case RDCREATE:
			if (rd_info[target].flags && RD_BUSY) {
				return -EBUSY;
			} else {
				/* allocate memory */
#if 0
				rd_info[target].size = 0;
#endif
				k = -1;
				for (i = 0; i <= (arg - 1) / ((SEG_SIZE / 1024) * P_SIZE); i++) {
					j = find_free_seg(); /* find free place in queue */
#ifdef DEBUG
					printk("rd_ioctl(): find_free_seg() = %d\n", j);
#endif
					if (j == -1) {
						rd_dealloc(target);
						return -ENOMEM;
					}
					if (i == 0)
						rd_info[target].index = j;
					
					if (i == (arg / ((SEG_SIZE / 1024) * P_SIZE)))
					/* size in 16 byte pagez = (arg % 64) * 64 */
						size = (arg % ((SEG_SIZE / 1024) * P_SIZE)) * ((SEG_SIZE / 1024) * P_SIZE);
						else
						size = SEG_SIZE;

					rd_segment[j].segment = mm_alloc(size);
					if (rd_segment[j].segment == -1) {
						rd_dealloc(target);
						return -ENOMEM;
					}
#ifdef DEBUG
					printk("rd_ioctl(): pass: %d, allocated %d pages, rd_segment[%d].segment = 0x%x\n", i, (int) size, j, rd_segment[j].segment);
#endif
					/* recalculate int size to reflect size in sectors, not pages */
					size = size / DIVISOR;

					rd_segment[j].seg_size = size; /* size in sectors */
#ifdef DEBUG
					printk("rd_ioctl(): rd_segment[%d].seg_size = %d sectors\n", j, rd_segment[j].seg_size);
#endif
					rd_info[target].size += rd_segment[j].seg_size; /* size in 512 B blocks */
					size = (long) rd_segment[j].seg_size * SECTOR_SIZE;
#ifdef DEBUG
					printk("rd_ioctl(): size = %ld\n", size);
#endif
					/* this terrible hack makes sure fmemset clears whole segment even if size == 64 KB :) */
					if (size != ((long) SEG_SIZE * (long) P_SIZE)) {
#ifdef DEBUG
						printk("rd_ioctl(): calling fmemset(%d, 0x%x, %d, %d) ..\n", 0, rd_segment[j].segment, 0, (int) size);
#endif
						fmemset(0, rd_segment[j].segment, 0, (int) size); /* clear seg_size * SECTOR_SIZE bytes */
					} else {
#ifdef DEBUG
						printk("rd_ioctl(): calling fmemset(%d, 0x%x, %d, %d) ..\n", 0, rd_segment[j].segment, 0, (int) (size / 2));
						printk("rd_ioctl(): calling fmemset(%d, 0x%x, %d, %d) ..\n", (int) (size / 2), rd_segment[j].segment, 0, (int) (size / 2));
#endif
						fmemset(0, rd_segment[j].segment, 0, (int) (size / 2)); /* we could hardcode 32768 instead of size / 2 here */
						fmemset((size / 2), rd_segment[j].segment, 0, (int) (size / 2));
					}

					if (k != -1)
						rd_segment[k].next = j; /* set link to next index */
					k = j;
				}
				rd_info[target].flags = RD_BUSY;
#ifdef DEBUG
				printk("rd_ioctl(): ramdisk %d created, size = %d blocks, index = %d\n", target, rd_info[target].size, rd_info[target].index);
#endif
			}
#ifdef DEBUG
			printk("rd_ioctl(): about to return(0);\n");
#endif
			return 0;
			break;
		case RDDESTROY:
			if (rd_info[target].flags && RD_BUSY) {
				invalidate_inodes(inode->i_rdev);
				invalidate_buffers(inode->i_rdev);
				rd_dealloc(target);
				rd_info[target].flags = 0;
				return 0;
			} else
				return -EINVAL;
			break;
	}
	return -EINVAL;
}

static void do_rd_request()
{
/* 	unsigned long count; */
	register char *buff;
	int target;
	rd_sector_t start; /* absolute offset from start of device */
	seg_t segnum; /* segment index; segment = rd_segment[segnum].segment */
	segext_t offset; /* relative offset (from start of segment) */

	while(1) {
		if (!CURRENT || CURRENT->rq_dev <0)
			return;

		INIT_REQUEST;

		if (CURRENT == NULL || CURRENT->rq_sector == (sector_t)-1)
			return;

		if (rd_initialised != 1) {
			end_request(0, CURRENT->rq_dev);
			continue;
		}

		start = (rd_sector_t)CURRENT->rq_sector;
		buff = CURRENT->rq_buffer;
		target = DEVICE_NR(CURRENT->rq_dev);

#ifdef DEBUG
		printk("do_rd_request(): target: %d, start: %ld\n", target, (long)start);
#endif

		/* FIXME (DONE): there is really no need for 3rd condition ... count isn't used anywhere, we only have 1024 byte requests */
#if 0 /* old code */
		if ((rd_info[target].flags != RD_BUSY) || (start >= rd_info[target].size) || (start + count >= rd_info[target].size)) {
#else
		if ((rd_info[target].flags != RD_BUSY) || (start >= rd_info[target].size)) {
#endif
			printd_rd("Bollocks request\n");
#ifdef DEBUG
			printk("do_rd_request: bad request on dev %d, flags: %d, size: %d, start: %d\n", target, rd_info[target].flags, rd_info[target].size, start);
#endif			
			end_request(0, CURRENT->rq_dev);
			continue;
		}
		offset = start; /* offset from segment start */
		segnum = rd_info[target].index; /* we want to know our starting index nr. */
#ifdef DEBUG 
		printk("do_rd_request(): index = %d\n", segnum);
#endif
		while (offset > rd_segment[segnum].seg_size) {
			offset -= rd_segment[segnum].seg_size; /* recalculate offset */
			segnum = rd_segment[segnum].next; /* point to next segment in linked list */
		}
#ifdef DEBUG
		printk("do_rd_request(): entry = %d, segment = 0x%x, offset = %d\n", segnum, rd_segment[segnum].segment, offset);
#endif
		if (CURRENT->rq_cmd == WRITE) {
			printd_rd1("RD_REQUEST writing to %ld\n", start);
			fmemcpy(rd_segment[segnum].segment, offset, get_ds(), buff, 1024);
		}
		if (CURRENT->rq_cmd == READ) {
			printd_rd1("RD_REQUEST reading from %ld\n", start);
			fmemcpy(get_ds(), buff, rd_segment[segnum].segment, offset, 1024);
		}
		end_request(1, CURRENT->rq_dev);
	}
}

#endif /* CONFIG_BLK_DEV_RAM */
