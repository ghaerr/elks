/*
 * ELKS implmentation of memory devices
 * /dev/null, /dev/zero, /dev/mem, /dev/kmem, etc...
 *
 * Heavily inspired by linux/drivers/char/mem.c
 */

/* for reference
 * /dev/mem refers to physical memory
 * /dev/kmem refers to _virtual_ address space
 * Currently these will be the same, but eventually, once ELKS has
 * EMS, etc, we'll want to change these.
 */

#include <linuxmt/config.h>

#ifdef CONFIG_CHAR_DEV_MEM

#include <linuxmt/kernel.h>
#include <linuxmt/major.h>
#include <linuxmt/fs.h>
#include <linuxmt/errno.h>
#include <linuxmt/mm.h>
#include <linuxmt/debug.h>
#include <linuxmt/mem.h>
#include <linuxmt/heap.h>

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
int memory_lseek(struct inode *inode, register struct file *filp,
		    loff_t offset, unsigned int origin)
{
    debugmem("mem_lseek()\n");
    switch (origin) {
    case 1:
	offset += filp->f_pos;
    case 0:
	if (offset >= 0)
	    break;
    default:
	return -EINVAL;
    }
    if (offset != filp->f_pos) {
	filp->f_pos = offset;

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
int null_lseek(struct inode *inode, struct file *filp,
		  off_t offset, int origin)
{
    debugmem("null_lseek()\n");
    filp->f_pos = 0;
    return 0;
}

size_t null_read(struct inode *inode, struct file *filp, char *data, size_t len)
{
    debugmem("null_read()\n");
    return 0;
}

size_t null_write(struct inode *inode, struct file *filp, char *data, size_t len)
{
    debugmem1("null write: ignoring %d bytes!\n", len);
    return (size_t)len;
}

/*
 * /dev/full code
 */
size_t full_read(struct inode *inode, struct file *filp, char *data, size_t len)
{
    debugmem("full_read()\n");
    filp->f_pos += len;
    return len;
}

size_t full_write(struct inode *inode, struct file *filp, char *data, size_t len)
{
    debugmem1("full_write: objecting to %d bytes!\n", len);
    return -ENOSPC;
}

/*
 * /dev/zero code
 */
size_t zero_read(struct inode *inode, struct file *filp, char *data, size_t len)
{
    debugmem("zero_read()\n");
    fmemsetb((word_t)data, current->t_regs.ds, 0, (word_t) len);
    filp->f_pos += len;
    return (size_t)len;
}

static unsigned short int split_seg_off(unsigned short int *offset,
			  long int posn)
{
    *offset = (unsigned short int) (((unsigned long int) posn) & 0xF);
    return (unsigned short int) (((unsigned long int) posn) >> 4);
}

/*
 * /dev/kmem (and currently also mem) code
 */
size_t kmem_read(struct inode *inode, register struct file *filp,
	      char *data, size_t len)
{
    unsigned short int sseg, soff;

    debugmem("[k]mem_read()\n");
    sseg = split_seg_off(&soff, filp->f_pos);
    debugmem3("Reading %u %p %p.\n", len, sseg, soff);
    fmemcpyb((byte_t *)data, current->t_regs.ds, (byte_t *)soff, sseg, (word_t) len);
    filp->f_pos += len;
    return (size_t) len;
}

size_t kmem_write(struct inode *inode, register struct file *filp,
	       char *data, size_t len)
{
    unsigned short int dseg, doff;

    debugmem("[k]mem_write()\n");

    dseg = split_seg_off(&doff, filp->f_pos);
    debugmem2("Writing to %d:%d\n", dseg, doff);
    fmemcpyb((byte_t *)doff, dseg, (byte_t *)data, current->t_regs.ds, (word_t) len);
    filp->f_pos += len;
    return len;
}

// Heap iterator callback

#ifdef HEAP_DEBUG

int heap_cb (heap_s * h)
{
	printk ("heap:%Xh:%u:%hxh\n",h, h->size, h->tag);
}

#endif /* HEAP_DEBUG */


int kmem_ioctl(struct inode *inode, struct file *file, int cmd, register char *arg)
{
    struct mem_usage mu;

    debugmem1("[k]mem_ioctl() %d\n", cmd);
    switch (cmd) {

#ifdef CONFIG_MODULES

    case MEM_GETMODTEXT:
	put_user((unsigned short int)module_init, (void *)arg);
	return 0;
    case MEM_GETMODDATA:
	put_user((unsigned short int)module_data, (void *)arg);
	return 0;

#endif

    case MEM_GETTASK:
	put_user((unsigned short int)task, (void *)arg);

#if 0

	/* Include this code to make ps dump memory info. The dmem()
	 * function must also be included in arch/i86/mm/malloc.c
	 */
	dmem();

#endif

	return 0;
    case MEM_GETCS:
	put_user((unsigned short int)kernel_cs, (void *)arg);
	return 0;
    case MEM_GETDS:
	put_user((unsigned short int)kernel_ds, (void *)arg);
	return 0;
    case MEM_GETUSAGE:
	mu.free_memory = mm_get_usage(MM_MEM, 0);
	mu.used_memory = mm_get_usage(MM_MEM, 1);

	memcpy_tofs(arg, &mu, sizeof(struct mem_usage));

#ifdef HEAP_DEBUG
	heap_iterate (heap_cb);
#endif /* HEAP_DEBUG */
	
	return 0;
    }
    return -EINVAL;
}

/*@-type@*/

static struct file_operations null_fops = {
    null_lseek,			/* lseek */
    null_read,			/* read */
    null_write,			/* write */
    NULL,			/* readdir */
    NULL,			/* select */
    NULL,			/* ioctl */
    NULL,			/* open */
    NULL			/* release */
#ifdef BLOAT_FS
	,
    NULL,			/* fsync */
    NULL,			/* check_media_change */
    NULL			/* revalidate */
#endif
};

static struct file_operations full_fops = {
    memory_lseek,		/* lseek */
    full_read,			/* read */
    full_write,			/* write */
    NULL,			/* readdir */
    NULL,			/* select */
    NULL,			/* ioctl */
    NULL,			/* open */
    NULL			/* release */
#ifdef BLOAT_FS
	,
    NULL,			/* fsync */
    NULL,			/* check_media_change */
    NULL			/* revalidate */
#endif
};

static struct file_operations zero_fops = {
    memory_lseek,		/* lseek */
    zero_read,			/* read */
    null_write,			/* write */
    NULL,			/* readdir */
    NULL,			/* select */
    NULL,			/* ioctl */
    NULL,			/* open */
    NULL			/* release */
#ifdef BLOAT_FS
	,
    NULL,			/* fsync */
    NULL,			/* check_media_change */
    NULL			/* revalidate */
#endif
};

static struct file_operations kmem_fops = {
    memory_lseek,		/* lseek */
    kmem_read,			/* read */
    kmem_write,			/* write */
    NULL,			/* readdir */
    NULL,			/* select */
    kmem_ioctl,			/* ioctl */
    NULL,			/* open */
    NULL			/* release */
#ifdef BLOAT_FS
	,
    NULL,			/* fsync */
    NULL,			/* check_media_change */
    NULL			/* revalidate */
#endif
};

/*
 * memory device open multiplexor
 */
int memory_open(register struct inode *inode, struct file *filp)
{
#ifdef DEBUG
    static char *mdev_nam[] = {

    /*  Unimplemented minors will print out the correct device name
     *  with a warning note.
     */

	"???",
	"mem",
	"kmem",
	"null",
	"port", /* not implemented */
	"zero",
	"???",  /* OBSOLETE core */
	"full"
    };
#endif
    static struct file_operations *mdev_fops[] = {
	NULL,		/*  */

    /*  The following two entries assume that virtual memory is identical
     *  to physical memory.
     */

	&kmem_fops,	/* DEV_MEM_MINOR */
	&kmem_fops,	/* DEV_KMEM_MINOR */
	&null_fops,	/* DEV_NULL_MINOR */
	NULL,		/* DEV_PORT_MINOR */
	&zero_fops,	/* DEV_ZERO_MINOR */
	NULL,		/* OBSOLETE core */
	&full_fops	/* DEV_FULL_MINOR */
    };
    unsigned int minor;

    minor = MINOR(inode->i_rdev);
    if ((minor > 7) || !mdev_fops[minor]) {
	printk("Device minor %d not supported.\n", minor);
	return -ENXIO;
    }
    debugmem2("memory_open: minor = %u; it's /dev/%s\n",
		minor, mdev_nam[minor]);
    filp->f_op = mdev_fops[minor];
    return 0;
}

static struct file_operations memory_fops = {
    NULL,			/* lseek */
    NULL,			/* read */
    NULL,			/* write */
    NULL,			/* readdir */
    NULL,			/* select */
    NULL,			/* ioctl */
    memory_open,		/* open */
    NULL			/* release */
#ifdef BLOAT_FS
	,
    NULL,			/* fsync */
    NULL,			/* check_media_change */
    NULL			/* revalidate */
#endif
};

/*@+type@*/

void mem_dev_init(void)
{
    if (register_chrdev(MEM_MAJOR, "mem", &memory_fops))
	printk("MEM: Unable to get major %d for memory devices\n", MEM_MAJOR);
}

#endif
