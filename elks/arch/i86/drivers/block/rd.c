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

#define SECTOR_SIZE 1024
#define SEG_SIZE 64 /* size in 1024 byte blocks */
#define MEM_SIZE SEG_SIZE * SECTOR_SIZE

static int rd_initialised = 0;
static struct rd_infot
{
	int index;
	int flags;
	int size;
} rd_info[8] = {0,0,0,};

static struct rd_segmentt
{
	int segment;
	int next;
	int seg_size;
} rd_segment[8] = {0,0,0,}; /* max 640 KB will be used for RAM disk(s) */

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
	rd_info[DEVICE_NR(inode->i_rdev)].flags = 0;
	return 0;
}

int find_free_seg()
{
	int i;
	for (i = 0; i < 8; i++)
		if (!rd_segment[i].segment)
			return i;
	return -1;
}

int rd_dealloc(target)
int target;
{
	int i, j;

	i = rd_info[target].index;
	i = rd_segment[i].segment;
	while (i != 0) {	
		j = i;
		mm_free(rd_segment[j].segment);
		rd_segment[j].segment = 0;
		i = rd_segment[j].next;
		rd_segment[j].next = 0;
	}
	rd_info[target].flags = 0;
	
	return 0;
}

static int rd_ioctl(inode, file, cmd, arg)
register struct inode *inode;
struct file *file;
unsigned int cmd;
unsigned int arg;
{
	int target = DEVICE_NR(inode->i_rdev);
	int i, j, k;
	int size;

	if (!suser())
		return -EPERM;
	printd_rd2("RD_IOCTL %d %s\n", target, (cmd ? "kill" : "make"));
	switch(cmd) {
		case RDCREATE:
			if (rd_info[target].flags && RD_BUSY) {
				return -EBUSY;
			} else {
				/* allocate memory */
				k = -1;
				for (i = 0; i <= arg / SEG_SIZE; i++) {
					j = find_free_seg(); /* find free place in queue */
					if (j == -1) {
						rd_dealloc(target);
						return -ENOMEM;
					}
					if (i == arg / SEG_SIZE)
						size = (arg * SECTOR_SIZE) % MEM_SIZE;
						else
						size = MEM_SIZE;
					
					if ((rd_segment[j].segment = mm_alloc(size, 0)) == -1) {
						rd_dealloc(target);
						return -ENOMEM;
					}
					rd_segment[j].seg_size = size;
					fmemset(0, rd_segment[j].segment, 0, MEM_SIZE);
					if (k != -1)
						rd_segment[k].next = j;
					k = j;
				}
				rd_info[target].flags = RD_BUSY;
				rd_info[target].size = arg;
			}
			return 0;
			break;
		case RDDESTROY:
			if (rd_info[target].flags && RD_BUSY) {
				rd_dealloc(rd_segment[rd_info[target].index].segment);
				invalidate_inodes(inode->i_rdev);
				invalidate_buffers(inode->i_rdev);
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
	unsigned long start;
	register char *buff;
	int target;
	int offset, segnum;

	while(1) {
		if (!CURRENT || CURRENT->rq_dev <0)
			return;

		INIT_REQUEST;

		if (CURRENT == NULL || CURRENT->rq_sector == -1)
			return;

		if (rd_initialised != 1) {
			end_request(0, CURRENT->rq_dev);
			continue;
		}

		start = CURRENT->rq_sector;
		buff = CURRENT->rq_buffer;
		target = DEVICE_NR(CURRENT->rq_dev);

		/* FIXME (DONE): there is really no need for 3rd condition ... count isn't used anywhere, we only have 1024 byte requests */
#if 0/* old code */
		if ((rd_info[target].flags != RD_BUSY) || (start >= rd_info[target].size) || (start + count >= rd_info[target].size)) {
#endif
		if ((rd_info[target].flags != RD_BUSY) || (start >= rd_info[target].size)) {
			printd_rd("Bollocks request\n");
			end_request(0, CURRENT->rq_dev);
			continue;
		}
		
		offset = start * 512; /* offset from segment start */
		segnum = rd_info[target].index; /* first segment number */
		while ((offset - rd_segment[segnum].seg_size) > 0) {
			offset -= rd_segment[segnum].seg_size; /* recalculate offset */
			segnum = rd_segment[segnum].next; /* point to next segment in linked list */
		}

		if (CURRENT->rq_cmd == WRITE) {
			printd_rd2("RD_REQUEST writing to %ld size %ld\n", start,count);
			fmemcpy(rd_segment[segnum].segment, offset, get_ds(), buff, 1024);
		}
		if (CURRENT->rq_cmd == READ) {
			printd_rd2("RD_REQUEST reading from %ld size %ld\n", start,count);
			fmemcpy(get_ds(), buff, rd_segment[segnum].segment, offset, 1024);
		}
		end_request(1, CURRENT->rq_dev);
	}
}

#endif /* CONFIG_BLK_DEV_RAM */
