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

struct device_struct 
{
	char * name;
	struct file_operations * fops;
};

static struct device_struct chrdevs[MAX_CHRDEV] = 
{
	{ NULL, NULL },
};

static struct device_struct blkdevs[MAX_BLKDEV] = {
	{ NULL, NULL },
};

#ifdef ONE_DAY_IN_THE_FUTURE
int get_device_list(page)
register char *page;
{
	int i;
	int len;

	len = sprintf(page, "Character devices:\n");
	for (i = 0; i < MAX_CHRDEV ; i++) {
		if (chrdevs[i].fops) {
			len += sprintf(page+len, "%2d %s\n", i, chrdevs[i].name);
		}
	}
	len += sprintf(page+len, "\nBlock devices:\n");
	for (i = 0; i < MAX_BLKDEV ; i++) {
		if (blkdevs[i].fops) {
			len += sprintf(page+len, "%2d %s\n", i, blkdevs[i].name);
		}
	}
	return len;
}
#endif

struct file_operations * get_blkfops(major)
unsigned int major;
{
	if (major >= MAX_BLKDEV)
		return NULL;
	return blkdevs[major].fops;
}
#if 0
struct file_operations * get_chrfops(major)
unsigned int major;
{
	if (major >= MAX_CHRDEV)
		return NULL;
	return chrdevs[major].fops;
}
#endif
int register_chrdev(major,name, fops)
unsigned int major;
char * name;
register struct file_operations *fops;
{
	register struct device_struct * dev = &chrdevs[major];

	if (major == 0) {
		for (major = MAX_CHRDEV-1; major > 0; major--) {
			if (dev->fops == NULL) {
				dev->name = name;
				dev->fops = fops;
				return major;
			}
		}
		return -EBUSY;
	}
	if (major >= MAX_CHRDEV)
		return -EINVAL;
	if (dev->fops && dev->fops != fops)
		return -EBUSY;
	dev->name = name;
	dev->fops = fops;
	return 0;
}

int register_blkdev(major,name,fops)
unsigned int major;
char * name;
register struct file_operations *fops;
{
	register struct device_struct * dev = &blkdevs[major];

	if (major == 0) {
		for (major = MAX_BLKDEV-1; major > 0; major--) {
			if (dev->fops == NULL) {
				dev->name = name;
				dev->fops = fops;
				return major;
			}
		}
		return -EBUSY;
	}
	if (major >= MAX_BLKDEV)
		return -EINVAL;
	if (dev->fops && dev->fops != fops)
		return -EBUSY;
	dev->name = name;
	dev->fops = fops;
	return 0;
}
#if 0
int unregister_chrdev(major,name)
unsigned int major;
char * name;
{
	if (major >= MAX_CHRDEV)
		return -EINVAL;
	if (!chrdevs[major].fops)
		return -EINVAL;
	if (strcmp(chrdevs[major].name, name))
		return -EINVAL;
	chrdevs[major].name = NULL;
	chrdevs[major].fops = NULL;
	return 0;
}

int unregister_blkdev(major,name)
unsigned int major;
char * name;
{
	if (major >= MAX_BLKDEV)
		return -EINVAL;
	if (!blkdevs[major].fops)
		return -EINVAL;
	if (strcmp(blkdevs[major].name, name))
		return -EINVAL;
	blkdevs[major].name = NULL;
	blkdevs[major].fops = NULL;
	return 0;
}
#endif
/*
 * This routine checks whether a removable media has been changed,
 * and invalidates all buffer-cache-entries in that case. This
 * is a relatively slow routine, so we have to try to minimize using
 * it. Thus it is called only upon a 'mount' or 'open'. This
 * is the best way of combining speed and utility, I think.
 * People changing diskettes in the middle of an operation deserve
 * to loose :-)
 */
#ifdef BLOAT_FS
int check_disk_change(dev)
kdev_t dev;
{
	int i;
	register struct file_operations * fops;

	i = MAJOR(dev);
	if (i >= MAX_BLKDEV || (fops = blkdevs[i].fops) == NULL)
		return 0;
	if (fops->check_media_change == NULL)
		return 0;
	if (!fops->check_media_change(dev))
		return 0;

	printk("VFS: Disk change detected on device %s\n",
		kdevname(dev));
	for (i=0 ; i<NR_SUPER ; i++)
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
int blkdev_open(inode,filp)
register struct inode * inode;
register struct file * filp;
{
	int i;

	i = MAJOR(inode->i_rdev);
	if (i >= MAX_BLKDEV || !blkdevs[i].fops)
		return -ENODEV;
	filp->f_op = blkdevs[i].fops;
	if (filp->f_op->open)
		return filp->f_op->open(inode,filp);
	return 0;
}	

/*
 * Dummy default file-operations: the only thing this does
 * is contain the open that then fills in the correct operations
 * depending on the special file...
 */
struct file_operations def_blk_fops = {
	NULL,		/* lseek */
	NULL,		/* read */
	NULL,		/* write */
	NULL,		/* readdir */
	NULL,		/* select */
	NULL,		/* ioctl */
	blkdev_open,	/* open */
	NULL,		/* release */
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
 * Called every time a character special file is opened
 */

int chrdev_open(inode,filp)
register struct inode * inode;
register struct file * filp;
{
	int i;

	i = MAJOR(inode->i_rdev);
	if (i >= MAX_CHRDEV || !chrdevs[i].fops)
		return -ENODEV;
	filp->f_op = chrdevs[i].fops;
	if (filp->f_op->open)
		return filp->f_op->open(inode,filp);
	return 0;
}

/*
 * Dummy default file-operations: the only thing this does
 * is contain the open that then fills in the correct operations
 * depending on the special file...
 */

struct file_operations def_chr_fops = 
{
	NULL,		/* lseek */
	NULL,		/* read */
	NULL,		/* write */
	NULL,		/* readdir */
	NULL,		/* select */
	NULL,		/* ioctl */
	chrdev_open,	/* open */
	NULL,		/* release */
};

struct inode_operations chrdev_inode_operations = 
{
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

char * kdevname(dev)
kdev_t dev;
{
	static char buffer[16];
	register char *hexof="0123456789ABCDEF";
	register char *bp=buffer;
	unsigned char b=MAJOR(dev);

	*bp++=hexof[b>>4];
	*bp++=hexof[b&15];
	b=MINOR(dev);
	*bp++=hexof[b>>4];
	*bp++=hexof[b&15];
	*bp=0;
	return buffer;
}

