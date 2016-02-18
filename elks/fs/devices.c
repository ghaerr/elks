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
#ifdef CONFIG_DEV_NAMES
    char *ds_name;
#endif
    struct file_operations *ds_fops;
};

static struct device_struct chrdevs[MAX_CHRDEV] = /*@i1@*/ {
#ifdef CONFIG_DEV_NAMES
    {NULL, NULL},
#else
    {NULL},
#endif
};

static struct device_struct blkdevs[MAX_BLKDEV] = /*@i1@*/ {
#ifdef CONFIG_DEV_NAMES
    {NULL, NULL},
#else
    {NULL},
#endif
};

struct file_operations *get_blkfops(unsigned int major)
{
    return (major >= MAX_BLKDEV)
	? NULL
	: blkdevs[major].ds_fops;
}

int register_chrdev(unsigned int major, char *name,
		    register struct file_operations *fops)
{
    register struct device_struct *dev = &chrdevs[major];

    if (major >= MAX_CHRDEV)
	return -EINVAL;
    if (dev->ds_fops && (dev->ds_fops != fops))
	return -EBUSY;
    dev->ds_fops = fops;

#ifdef CONFIG_DEV_NAMES
    dev->ds_name = name;
#endif

    return 0;
}

int register_blkdev(unsigned int major, char *name,
		    register struct file_operations *fops)
{
    register struct device_struct *dev = &blkdevs[major];

    if (major >= MAX_BLKDEV)
	return -EINVAL;
    if (dev->ds_fops && dev->ds_fops != fops)
	return -EBUSY;
    dev->ds_fops = fops;

#ifdef CONFIG_DEV_NAMES
    dev->ds_name = name;
#endif

    return 0;
}

#ifdef BLOAT_FS

/*
 * This routine checks whether a removable media has been changed,
 * and invalidates all buffer-cache-entries in that case. This
 * is a relatively slow routine, so we have to try to minimize using
 * it. Thus it is called only upon a 'mount' or 'open'. This
 * is the best way of combining speed and utility, I think.
 * People changing diskettes in the middle of an operation deserve
 * to lose :-)
 */
int check_disk_change(kdev_t dev)
{
    register struct file_operations *fops;
    int i;

    i = MAJOR(dev);
    if (i >= MAX_BLKDEV || (fops = blkdevs[i].ds_fops) == NULL)
	return 0;
    if (fops->check_media_change == NULL)
	return 0;
    if (!fops->check_media_change(dev))
	return 0;

    printk("VFS: Disk change detected on device %s\n", kdevname(dev));
    for (i = 0; i < NR_SUPER; i++)
	if (super_blocks[i].s_dev == dev)
	    put_super(super_blocks[i].s_dev);
    invalidate_inodes(dev);
    invalidate_buffers(dev);

    if (fops->revalidate)
	fops->revalidate(dev);
    return 1;
}

#endif

/*
 * Called every time a block special file is opened
 */

int blkdev_open(struct inode *inode, struct file *filp)
{
    register struct file_operations *fop;
    register char *pi;

    pi = (char *)MAJOR(inode->i_rdev);
    fop = blkdevs[(unsigned int)pi].ds_fops;
    if ((unsigned int)pi >= MAX_BLKDEV || !fop)
	return -ENODEV;
    filp->f_op = fop;
    return (fop->open)
	? fop->open(inode, filp)
	: 0;
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
#ifdef BLOAT_FS
    NULL,			/* bmap */
#endif
    NULL,			/* truncate */
#ifdef BLOAT_FS
    NULL			/* permission */
#endif
};

/*
 * Called every time a character special file is opened.
 */

static int chrdev_open(struct inode *inode, struct file *filp)
{
    register struct file_operations *fop;
    register char *pi;

    pi = (char *)MAJOR(inode->i_rdev);
    fop = chrdevs[(unsigned int)pi].ds_fops;
    if ((unsigned int)pi >= MAX_CHRDEV || !fop)
	return -ENODEV;
    filp->f_op = fop;
    return (fop->open)
	? fop->open(inode, filp)
	: 0;
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
    NULL,			/* release */
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
#ifdef BLOAT_FS
    NULL,			/* bmap */
#endif
    NULL,			/* truncate */
#ifdef BLOAT_FS
    NULL			/* permission */
#endif
};

/*
 * Print device name (in decimal, hexadecimal or symbolic) -
 * at present hexadecimal only.
 * Note: returns pointer to static data!
 */

extern char *hex_string;	/* It lives in kernel/printk.c. */

char *kdevname(kdev_t dev)
{
#if (MINORBITS == 8) && (MINORMASK == 255)
    static char buffer[5];
    register char *bp = buffer + 4;
    *bp = 0;
    do {
	*--bp = hex_string[dev & 0xf];
	dev >>= 4;
    } while (bp > buffer);
#else
    static char buffer[16];
/*      register char *hexof = "0123456789ABCDEF"; */
    register char *hexof = hex_string;
    register char *bp = buffer;
    kdev_t b = MAJOR(dev);

    *bp++ = hexof[b >> 4];
    *bp++ = hexof[b & 15];
    b = MINOR(dev);
    *bp++ = hexof[b >> 4];
    *bp++ = hexof[b & 15];
    *bp = 0;
#endif
    return buffer;
}
