/*
 * ELKS implmentation of memory devices
 * /dev/null, /dev/port, /dev/zero, /dev/kmem
 *
 * Heavily inspired by linux/drivers/char/mem.c
 */

/* for reference
 * /dev/kmem refers to physical memory
 * /dev/port refers to hardware ports <Marcin.Laszewski@gmail.com>
 */

#include <linuxmt/config.h>

#ifdef CONFIG_CHAR_DEV_MEM

#include <linuxmt/kernel.h>
#include <linuxmt/major.h>
#include <linuxmt/fs.h>
#include <linuxmt/errno.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/debug.h>
#include <linuxmt/mem.h>
#include <linuxmt/heap.h>
#include <linuxmt/timer.h>
#include <linuxmt/init.h>

#include <arch/io.h>
#include <arch/segment.h>

#define DEV_MEM_MINOR		1       /* unused */
#define DEV_KMEM_MINOR		2
#define DEV_NULL_MINOR		3
#define DEV_PORT_MINOR		4
#define DEV_ZERO_MINOR		5
#define DEV_FULL_MINOR		6       /* unused */

/*
 * generally useful code...
 */
static int memory_lseek(struct inode *inode, struct file *filp, loff_t offset,
        unsigned int origin)
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
    }
    return 0;
}

/*
 * /dev/null code
 */
int null_lseek(struct inode *inode, struct file *filp, off_t offset, int origin)
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
    debugmem("null write: ignoring %d bytes!\n", len);
    return len;
}

/*
 * /dev/port code
 */
#if defined(CONFIG_CHAR_DEV_MEM_PORT_READ) || defined(CONFIG_CHAR_DEV_MEM_PORT_WRITE)
int port_lseek(struct inode *inode, struct file *filp, off_t offset, int origin)
{
    debugmem("port_lseek()\n");

    switch (origin)
    {
        case 0: /* SEEK_SET */
            break;

        case 1: /* SEEK_CUR */
            offset += filp->f_pos;
            break;

        case 2: /* SEEK_END */
            offset = port_MAX + offset;
            break;

        default:
            return -EINVAL;
    }

    if(offset < 0)
        offset = 0;
    else if(offset > port_MAX)
        offset = port_MAX;

    filp->f_pos = offset;
    debugmem("port_lseek: 0x%02X\n", (unsigned)filp->f_pos);

    return 0;
}
#endif

#if defined(CONFIG_CHAR_DEV_MEM_PORT_READ)
size_t port_read(struct inode *inode, struct file *filp, char *data, size_t len)
{
    size_t i;

    debugmem("port_read()\n");

    for (i = 0; i < len && filp->f_pos < port_MAX; i++)
    {
        put_user_char(inb((unsigned)filp->f_pos++), (void *)(data++));
        debugmem("port_read(0x%02X) = %02X\n",
            (unsigned)filp->f_pos - 1,
            (unsigned)(((unsigned char *)data)[-1]));
    }

    return i;
}
#else
#	define	port_read	NULL
#endif

#if defined(CONFIG_CHAR_DEV_MEM_PORT_WRITE)
size_t port_write(struct inode *inode, struct file *filp, char *data, size_t len)
{
    size_t i;

    debugmem("port_write\n");

    for (i = 0; i < len && filp->f_pos < port_MAX; i++)
    {
        debugmem("port: write(0x%02X) = %02X\n",
            (unsigned)filp->f_pos,
            (unsigned)get_user_char((void *)data));
        outb(get_user_char((void *)data++), (unsigned)filp->f_pos++);
    }

    return i;
}
#else
#	define	port_write	NULL
#endif

#if UNUSED
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
    debugmem("full_write: objecting to %d bytes!\n", len);
    return -ENOSPC;
}
#endif

/*
 * /dev/zero code
 */
size_t zero_read(struct inode *inode, struct file *filp, char *data, size_t len)
{
    debugmem("zero_read()\n");
    fmemsetb(data, current->t_regs.ds, 0, len);
    filp->f_pos += len;
    return (size_t)len;
}

static unsigned short int split_seg_off(unsigned short int *offset, long int posn)
{
    *offset = (unsigned short int) (((unsigned long int) posn) & 0xF);
    return (unsigned short int) (((unsigned long int) posn) >> 4);
}

/*
 * /dev/kmem code
 */
size_t kmem_read(struct inode *inode, register struct file *filp, char *data, size_t len)
{
    unsigned short int sseg, soff;

    debugmem("kmem_read()\n");
    sseg = split_seg_off(&soff, filp->f_pos);
    debugmem("Reading %u %p %p.\n", len, sseg, soff);
    fmemcpyb((byte_t *)data, current->t_regs.ds, (byte_t *)soff, sseg, (word_t) len);
    filp->f_pos += len;
    return (size_t) len;
}

size_t kmem_write(struct inode *inode, register struct file *filp, char *data, size_t len)
{
    unsigned short int dseg, doff;

    debugmem("kmem_write()\n");
    dseg = split_seg_off(&doff, filp->f_pos);
    debugmem("Writing to %d:%d\n", dseg, doff);
    fmemcpyb((byte_t *)doff, dseg, (byte_t *)data, current->t_regs.ds, (word_t) len);
    filp->f_pos += len;
    return len;
}

int kmem_ioctl(struct inode *inode, struct file *file, int cmd, char *arg)
{
    unsigned short retword;
    struct mem_usage mu;

    switch (cmd) {
    case MEM_GETTASK:
	retword = (unsigned short)task;
	break;
    case MEM_GETMAXTASKS:
	retword = max_tasks;
	break;
    case MEM_GETCS:
	retword = kernel_cs;
	break;
    case MEM_GETDS:
	retword = kernel_ds;
	break;
    case MEM_GETFARTEXT:
        retword = (unsigned)((long)kernel_init >> 16);
        break;
    case MEM_GETUSAGE:
	mm_get_usage (&(mu.free_memory), &(mu.used_memory));
	memcpy_tofs(arg, &mu, sizeof(struct mem_usage));
	return 0;
    case MEM_GETHEAP:
	retword = (unsigned short) &_heap_all;
	break;
    case MEM_GETUPTIME:
#ifdef CONFIG_CPU_USAGE
	retword = (unsigned short) &uptime;
	break;
#endif
    default:
	return -EINVAL;
    }
    put_user(retword, arg);
    return 0;
}

static struct file_operations null_fops = {
    null_lseek,			/* lseek */
    null_read,			/* read */
    null_write,			/* write */
    NULL,			/* readdir */
    NULL,			/* select */
    NULL,			/* ioctl */
    NULL,			/* open */
    NULL			/* release */
};

#if defined(CONFIG_CHAR_DEV_MEM_PORT_READ) || defined(CONFIG_CHAR_DEV_MEM_PORT_WRITE)
static struct file_operations port_fops = {
    port_lseek,			/* lseek */
    port_read,			/* read */
    port_write,			/* write */
    NULL,			/* readdir */
    NULL,			/* select */
    NULL,			/* ioctl */
    NULL,			/* open */
    NULL			/* release */
};
#endif

static struct file_operations zero_fops = {
    memory_lseek,		/* lseek */
    zero_read,			/* read */
    null_write,			/* write */
    NULL,			/* readdir */
    NULL,			/* select */
    NULL,			/* ioctl */
    NULL,			/* open */
    NULL			/* release */
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
};

#if UNUSED
static struct file_operations full_fops = {
    memory_lseek,		/* lseek */
    full_read,			/* read */
    full_write,			/* write */
    NULL,			/* readdir */
    NULL,			/* select */
    NULL,			/* ioctl */
    NULL,			/* open */
    NULL			/* release */
};
#endif

/*
 * memory device open multiplexor
 */
int memory_open(register struct inode *inode, struct file *filp)
{
    static struct file_operations *mdev_fops[] = {
	NULL,
	&kmem_fops,	/* DEV_MEM_MINOR */
	&kmem_fops,	/* DEV_KMEM_MINOR */
	&null_fops,	/* DEV_NULL_MINOR */
#if defined(CONFIG_CHAR_DEV_MEM_PORT_READ) || defined(CONFIG_CHAR_DEV_MEM_PORT_WRITE)
	&port_fops,	/* DEV_PORT_MINOR */
#else
        NULL,
#endif
	&zero_fops,	/* DEV_ZERO_MINOR */
#if UNUSED
	&full_fops	/* DEV_FULL_MINOR */
#endif
    };
    unsigned int minor;

    minor = MINOR(inode->i_rdev);
    if (minor > 5 || !mdev_fops[minor])
	return -ENXIO;
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
};

void INITPROC mem_dev_init(void)
{
    register_chrdev(MEM_MAJOR, "mem", &memory_fops);
}
#endif
