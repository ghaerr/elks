#include <linuxmt/config.h>
#include <linuxmt/rd.h>
#include <linuxmt/major.h>
#include <linuxmt/kernel.h>
#include <linuxmt/debug.h>
#include <errno.h>

#ifdef CONFIG_BLK_DEV_RAM

#define MAJOR_NR RAM_MAJOR

#define RAMDISK
#include "blk.h"

#define NUM_SECTS 128
#define MEM_SIZE NUM_SECTS*32
#define SEG_SIZE 16

static int rd_initialised = 0;
static int rd_busy[8] = {0,0,0,0,0,0,0,0};
static int rd_segment[8] = {0,0,0,0,0,0,0,0};

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

	printk("rd driver Copyright (C) 1997 Alistair Riddoch\n");
	if ((i = register_blkdev(MAJOR_NR, DEVICE_NAME, &rd_fops)) == 0) {
		blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
		/* blksize_size[MAJOR_NR] = 1024; */
		/* read_ahead[MAJOR_NR] = 2; */
		rd_initialised = 1;
	} else {
		printk("RD failed to register.\n");
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
/*	if (rd_busy[target])
		return (-EBUSY); */
	return 0;
}

static int rd_release(inode, filp)
struct inode *inode;
struct file *filp;
{
	printd_rd("RD_RELEASE \n");
	rd_busy[DEVICE_NR(inode->i_rdev)] = 0;
	return 0;
}

static int rd_ioctl(inode, file, cmd, arg)
register struct inode *inode;
struct file *file;
unsigned int cmd;
unsigned int arg;
{
	int target = DEVICE_NR(inode->i_rdev);

	if (!suser())
		return -EPERM;
	printd_rd2("RD_IOCTL %d %s\n", target, (cmd ? "kill" : "make"));
	switch(cmd) {
		case RDCREATE:
			if (rd_segment[target]) {
				return -EBUSY;
			} else if ((rd_segment[target] = mm_alloc(MEM_SIZE,0)) == -1)
				return -ENOMEM;
			fmemset(0, rd_segment[target], 0, MEM_SIZE * SEG_SIZE);
			return 0;
			break;
		case RDDESTROY:
			if (rd_segment[target]) {
				mm_free(rd_segment[target]);
				rd_segment[target] = NULL;
				invalidate_inodes(inode->i_rdev);
				invalidate_buffers(inode->i_rdev);
				return 0;
			} else
				return -EINVAL;
			break;
	}
	return -EINVAL;
}

static void do_rd_request()
{
	unsigned long count;
	unsigned long start;
	register char *buff;
	int target;

	while(1) {
		if (!CURRENT || CURRENT->dev <0)
			return;

		INIT_REQUEST;

		if (CURRENT == NULL || CURRENT->sector == -1)
			return;

		if (rd_initialised != 1) {
			end_request(0);
			continue;
		}

		count = CURRENT->nr_sectors;
		start = CURRENT->sector;
		buff = CURRENT->buffer;
		target = DEVICE_NR(CURRENT->dev);

		if ((rd_segment[target] == NULL) || (start >= NUM_SECTS) || (start + count >= NUM_SECTS)) {
			printd_rd("Bollocks request\n");
			end_request(0);
			continue;
		}
		if (CURRENT->cmd == WRITE) {
			printd_rd2("RD_REQUEST writing to %ld size %ld\n", start,count);
			fmemcpy(rd_segment[target], start * 512, get_ds(), buff, count * 512);
		}
		if (CURRENT->cmd == READ) {
			printd_rd2("RD_REQUEST reading from %ld size %ld\n", start,count);
			fmemcpy(get_ds(), buff, rd_segment[target], start * 512, count * 512);
		}
		end_request(1);
	}
}

#endif /* CONFIG_BLK_DEV_RAM */
