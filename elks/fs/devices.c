/*
 *  linux/fs/devices.c
 *
 * (C) 1993 Matthias Urlichs -- collected common code and tables.
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/types.h>
#include <linuxmt/fs.h>
#include <linuxmt/major.h>
#include <linuxmt/string.h>
#include <linuxmt/sched.h>
#include <linuxmt/stat.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/errno.h>

struct device_struct {
    struct file_operations *ds_fops;
};

/* No initializer to put it in the BSS section (zero'ed at loading) */
static struct device_struct chrdevs[MAX_CHRDEV];

/* No initializer to put it in the BSS section (zero'ed at loading) */
static struct device_struct blkdevs[MAX_BLKDEV];

struct file_operations *get_blkfops(unsigned int major)
{
    return (major >= MAX_BLKDEV) ? NULL : blkdevs[major].ds_fops;
}

int register_chrdev(unsigned int major, const char *name, struct file_operations *fops)
{
    register struct device_struct *dev = &chrdevs[major];

    if (major >= MAX_CHRDEV) return -EINVAL;
    if (dev->ds_fops && (dev->ds_fops != fops)) return -EBUSY;
    dev->ds_fops = fops;
    return 0;
}

int register_blkdev(unsigned int major, const char *name, struct file_operations *fops)
{
    register struct device_struct *dev = &blkdevs[major];

    if (major >= MAX_BLKDEV) return -EINVAL;
    if (dev->ds_fops && dev->ds_fops != fops) return -EBUSY;
    dev->ds_fops = fops;
    return 0;
}

/*
 * Called every time a block special file is opened
 */

int blkdev_open(struct inode *inode, struct file *filp)
{
    register struct file_operations *fop;
    int i;

    i = MAJOR(inode->i_rdev);
    if (i >= MAX_BLKDEV || !(fop = blkdevs[i].ds_fops)) return -ENODEV;
    filp->f_op = fop;
    return (fop->open) ? fop->open(inode, filp) : 0;
}

/*
 * Dummy default file-operations: the only thing this does
 * is contain the open that then fills in the correct operations
 * depending on the special file...
 */
struct file_operations def_blk_fops = {
    NULL,			/* lseek */
    NULL,			/* read */
    NULL,			/* write */
    NULL,			/* readdir */
    NULL,			/* select */
    NULL,			/* ioctl */
    blkdev_open,		/* open */
    NULL,			/* release */
};

struct inode_operations blkdev_inode_operations = {
    &def_blk_fops,		/* default file operations */
    NULL,			/* create */
    NULL,			/* lookup */
    NULL,			/* link */
    NULL,			/* unlink */
    NULL,			/* symlink */
    NULL,			/* mkdir */
    NULL,			/* rmdir */
    NULL,			/* mknod */
    NULL,			/* readlink */
    NULL,			/* follow_link */
    NULL,			/* getblk */
    NULL			/* truncate */
};

/*
 * Called every time a character special file is opened.
 */

static int chrdev_open(struct inode *inode, struct file *filp)
{
    register struct file_operations *fop;
    int i;

    i = MAJOR(inode->i_rdev);
    if (i >= MAX_CHRDEV || !(fop = chrdevs[i].ds_fops)) return -ENODEV;
    filp->f_op = fop;
    return (fop->open) ? fop->open(inode, filp) : 0;
}

/*
 * Dummy default file-operations: the only thing this does
 * is contain the open that then fills in the correct operations
 * depending on the special file...
 */

struct file_operations def_chr_fops = {
    NULL,			/* lseek */
    NULL,			/* read */
    NULL,			/* write */
    NULL,			/* readdir */
    NULL,			/* select */
    NULL,			/* ioctl */
    chrdev_open,		/* open */
    NULL			/* release */
};

struct inode_operations chrdev_inode_operations = {
    &def_chr_fops,		/* default file operations */
    NULL,			/* create */
    NULL,			/* lookup */
    NULL,			/* link */
    NULL,			/* unlink */
    NULL,			/* symlink */
    NULL,			/* mkdir */
    NULL,			/* rmdir */
    NULL,			/* mknod */
    NULL,			/* readlink */
    NULL,			/* follow_link */
    NULL,			/* getblk */
    NULL			/* truncate */
};
