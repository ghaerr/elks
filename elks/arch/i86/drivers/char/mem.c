/*
 * ELKS implmentation of memory devices
 * /dev/null, /dev/zero, /dev/mem, /dev/kmem, etc...
 *
 * Heavily inspired by linux/drivers/char/mem.c
 */

/* for reference
 * /dev/mem refers to physical memory
 * /dev/kmem refers to _virtual_ address space
 * Currently these will be the same, bu eventually, once ELKS has
 * EMS, etc, we'll want to change these.
 */

#include <linuxmt/config.h>

#ifdef CONFIG_CHAR_DEV_MEM
#include <linuxmt/major.h>
#include <linuxmt/fs.h>
#include <linuxmt/errno.h>
#include <linuxmt/mm.h>
#include <linuxmt/debug.h>
#include <linuxmt/mem.h>
#include <arch/segment.h>

#define DEV_MEM_MINOR		1
#define DEV_KMEM_MINOR		2
#define DEV_NULL_MINOR		3
#define DEV_PORT_MINOR		4
#define DEV_ZERO_MINOR		5
#define DEV_FULL_MINOR		7
#define DEV_RANDOM_MINOR	8
#define DEV_URANDOM_MINOR	9

/*
 * generally useful code...
 */
int memory_lseek(inode, filp, offset, origin)
struct inode * inode;
register struct file * filp;
off_t offset;
unsigned int origin;
{
	long int tmp = -1;

	printd_mem("mem_lseek()\n");
	switch(origin) {
		case 0:
			tmp = offset;
			break;
		case 1:
			tmp = filp->f_pos + offset;
			break;
		default: 
			return -EINVAL;
	}
	if (tmp < 0)
		return -EINVAL;
	if (tmp != filp->f_pos) {
		filp->f_pos = tmp;
#ifdef BLOAT_FS
		filp->f_reada = 0;
		filp->f_version = ++event;
#endif
	}
	return 0;	
}



/*
 * /dev/null code
 */
int null_lseek(inode, filp, offset, origin)
struct inode * inode;
struct file * filp;
off_t offset;
int origin;
{
	printd_mem("null_lseek()\n");
	return (filp->f_pos = 0);
}

int null_read(inode, filp, data, len)
struct inode * inode;
struct file * filp;
char * data;
int len;
{
	printd_mem("null_read()\n");
	return 0;
}

int null_write(inode, filp, data, len)
struct inode * inode;
struct file * filp;
char * data;
int len;
{
	printd_mem1("null write: ignoring %d bytes!\n", len);
	return len;
}

static struct file_operations null_fops = {
	null_lseek,		/* lseek */
	null_read,		/* read */
	null_write,		/* write */
	NULL,			/* readdir */
	NULL,			/* select */
	NULL,			/* ioctl */
	NULL,			/* open */
	NULL,			/* release */
#ifdef BLOAT_FS
	NULL,			/* fsync */
	NULL,			/* check_media_change */
	NULL			/* revalidate */
#endif
};


/*
 * /dev/full code
 */
int full_read(inode, filp, data, len)
struct inode * inode;
struct file * filp;
char * data;
int len;
{
	printd_mem("full_read()\n");
	filp->f_pos += len;
	return len;
}

int full_write(inode, filp, data, len)
struct inode * inode;
struct file * filp;
char * data;
int len;
{
	printd_mem1("full_write: objecting to %d bytes!\n", len);
	return -ENOSPC;
}

static struct file_operations full_fops = {
	memory_lseek,		/* lseek */
	full_read,		/* read */
	full_write,		/* write */
	NULL,			/* readdir */
	NULL,			/* select */
	NULL,			/* ioctl */
	NULL,			/* open */
	NULL,			/* release */
#ifdef BLOAT_FS
	NULL,			/* fsync */
	NULL,			/* check_media_change */
	NULL			/* revalidate */
#endif
};


/*
 * /dev/zero code
 */
int zero_read(inode, filp, data, len)
struct inode * inode;
struct file * filp;
char * data;
int len;
{
	printd_mem("zero_read()\n");
	fmemset(data, current->mm.dseg, 0, len);
	filp->f_pos +=len;
	return len;
}

static struct file_operations zero_fops = {
	memory_lseek,		/* lseek */
	zero_read,		/* read */
	null_write,		/* write */
	NULL,			/* readdir */
	NULL,			/* select */
	NULL,			/* ioctl */
	NULL,			/* open */
	NULL,			/* release */
#ifdef BLOAT_FS
	NULL,			/* fsync */
	NULL,			/* check_media_change */
	NULL			/* revalidate */
#endif
};

/*
 * /dev/kmem (and currently also mem) code
 */
int kmem_read(inode, filp, data, len)
struct inode * inode;
register struct file * filp;
char * data;
unsigned int len;
{
	unsigned int sseg, soff;

	printd_mem("[k]mem_read()\n");
	soff = (filp->f_pos) & 0xfL;
	sseg = (filp->f_pos) >> 4;
/*	printk("Reading %d %p %p.\n", len, sseg, soff); */
	fmemcpy(current->mm.dseg, data, sseg, soff, len);
	filp->f_pos += len;
	return len;
}

int kmem_write(inode, filp, data, len)
struct inode * inode;
register struct file * filp;
char * data;
unsigned int len;
{
	unsigned int dseg, doff;
	printd_mem("[k]mem_write()\n");
	doff = (filp->f_pos) & 0xfL;
	dseg = (filp->f_pos) >> 4;
	printd_mem1("Writing to %d:", dseg);
	printd_mem1("%d\n", doff);
	fmemcpy(dseg, doff, current->mm.dseg, data, len);
	filp->f_pos += len;
	return len;
}

unsigned int kmem_ioctl(inode, file, cmd, arg) 
struct inode *inode;
struct file *file;
int cmd;
char *arg;
{
	char *i;
	printd_mem1("[k]mem_ioctl() %d\n",cmd);
	switch(cmd) {
#ifdef CONFIG_MODULE
		case MEM_GETMODTEXT:
			i = (char *)module_init;
			memcpy_tofs(arg, &i, 2);
			return 0;
		break;
		case MEM_GETMODDATA:
			i = (char *)module_data;
			memcpy_tofs(arg, &i, 2);
			return 0;
		break;
#endif
		case MEM_GETTASK:
			i = (char *)task;
			memcpy_tofs(arg, &i, 2);
#if 0
/* ****** I am not here	*/ dmem();
#endif
			return 0;
		case MEM_GETCS:
			i = get_cs();
			memcpy_tofs(arg, &i, 2);
			return 0;
		break;
		case MEM_GETDS:
			i = get_ds();
			memcpy_tofs(arg, &i, 2);
			return 0;
		break;
	}
	return -EINVAL;
}

static struct file_operations kmem_fops = {
	memory_lseek,		/* lseek */
	kmem_read,		/* read */
	kmem_write,		/* write */
	NULL,			/* readdir */
	NULL,			/* select */
	kmem_ioctl,		/* ioctl */
	NULL,			/* open */
	NULL,			/* release */
#ifdef BLOAT_FS
	NULL,			/* fsync */
	NULL,			/* check_media_change */
	NULL			/* revalidate */
#endif
};

/*
 * memory device open multiplexor
 */
int memory_open(inode, filp)
struct inode * inode;
register struct file * filp;
{
	printd_mem1("memory_open: minor = %d; it's /dev/", (int)MINOR(inode->i_rdev));
	switch(MINOR(inode->i_rdev)) {
		case DEV_NULL_MINOR:
			filp->f_op = & null_fops;
			printd_mem("null!\n");
			return 0;
		case DEV_FULL_MINOR:
			filp->f_op = & full_fops;
			printd_mem("full!\n");
			return 0;
		case DEV_ZERO_MINOR:
			filp->f_op = & zero_fops;
			printd_mem("zero!\n");
			return 0;
		case DEV_KMEM_MINOR:
		case DEV_MEM_MINOR: /* for now - assumes virt mem == phys mem */
			filp->f_op = & kmem_fops;
			printd_mem("kmem!\n");
			return 0;
		default:
	printd_mem("???\n");
		return -ENXIO;
	}
}


static struct file_operations memory_fops = {
	NULL,			/* lseek */
	NULL,			/* read */
	NULL,			/* write */
	NULL,			/* readdir */
	NULL,			/* select */
	NULL,			/* ioctl */
	memory_open,		/* open */
	NULL,			/* release */
#ifdef BLOAT_FS
	NULL,			/* fsync */
	NULL,			/* check_media_change */
	NULL			/* revalidate */
#endif
};

void mem_dev_init()
{
	if(register_chrdev(MEM_MAJOR, "mem", &memory_fops))
		printd_mem1("unable to get major %d for memory devices\n", MEM_MAJOR);
}

#endif /* CONFIG_CHAR_DEV_MEM */
