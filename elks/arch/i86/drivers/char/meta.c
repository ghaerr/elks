/*
 * arch/i86/drivers/char/meta.c
 *
 * Copyright (C) 1999 by Alistair Riddoch
 *
 * ELKS meta driver for user spoace device drivers (UDDs)
 */

#include <linuxmt/config.h>

#ifdef CONFIG_DEV_META
#include <linuxmt/major.h>
#include <linuxmt/fs.h>
#include <linuxmt/errno.h>
#include <linuxmt/mm.h>
#include <linuxmt/udd.h>
#include <linuxmt/debug.h>

#define MAJOR_NR	MISC_MAJOR

#define METADISK

#include "../block/blk.h"

struct ud_driver drivers[MAX_UDD];

struct ud_request requests[MAX_UDR];

static int meta_initialised = 0;

void meta_lseek() {}
void meta_read() {}
void meta_write() {}
void meta_select() {}
void meta_ioctl() {}
void meta_open() {}
void meta_release() {}
void do_meta_request() {}

static struct file_operations meta_chr_fops =
{
	meta_lseek,	/* lseek */
	meta_read,	/* read */
	meta_write,	/* write */
	NULL,		/* readdir */
	meta_select,	/* select */
	meta_ioctl,	/* ioctl */
	meta_open,	/* open */
	meta_release,	/* release */
#ifdef BLOAT_FS
	NULL,
	NULL,
	NULL
#endif
};

static struct file_operations meta_blk_fops =
{
	NULL,	/* lseek */
	block_read,	/* read */
	block_write,	/* write */
	NULL,		/* readdir */
	NULL,		/* select */
/* Not sure whether these have to be the same of re-written */
	meta_ioctl,	/* ioctl */
	meta_open,	/* open */
	meta_release,	/* release */
#ifdef BLOAT_FS
	NULL,
	NULL,
	NULL
#endif
};

void meta_init()
{
	int i;

	printk("meta driver Copyright (C) 1999 Alistair Riddoch\n");
	printk("meta personalities: ");
	if ((i = register_blkdev(MAJOR_NR, DEVICE_NAME, &meta_blk_fops)) == 0) {
		blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
		meta_initialised = UDD_BLK_DEV;
		printk("block ");
	}
	if ((i = register_chrdev(MAJOR_NR, DEVICE_NAME, &meta_chr_fops)) == 0) {
		meta_initialised |= UDD_CHR_DEV;
		printk("char ");
	}
	/* Can we do a tty personality ? */
	/* I think we can */
}

#endif /* CONFIG_DEV_META */
