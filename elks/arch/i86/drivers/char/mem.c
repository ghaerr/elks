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

#define DEV_MEM_MINOR           1       /* unused */
#define DEV_KMEM_MINOR          2
#define DEV_NULL_MINOR          3
#define DEV_PORT_MINOR          4
#define DEV_ZERO_MINOR          5
#define DEV_MAX_MINOR           5

/*
 * /dev/null code
 */
static int null_lseek(struct inode *inode, struct file *filp, loff_t offset, int origin)
{
    filp->f_pos = 0;
    return 0;
}

static size_t null_read(struct inode *inode, struct file *filp, char *data, size_t len)
{
    return 0;
}

static size_t null_write(struct inode *inode, struct file *filp, char *data, size_t len)
{
    return len;
}

/*
 * /dev/zero code
 */
static size_t zero_read(struct inode *inode, struct file *filp, char *data, size_t len)
{
    fmemsetb(data, current->t_regs.ds, 0, len);
    filp->f_pos += len;
    return len;
}

/*
 * /dev/kmem code
 */
#ifdef CONFIG_286_PMODE
/* Userland seeks to LINEARADDRESS(off, seg) = (seg << 4) + off, where seg came
 * from MEM_GETDS -- in PM that is the KERNEL_DS selector, so the packing is not
 * a physical address and (pos >> 4) does not reconstruct a loadable segment.
 * Undo the known composition instead: strip the (KERNEL_DS << 4) bias and access
 * the kernel data segment via its selector. Reads composed with other segment
 * values (e.g. a process's SS from its task struct) are not supported in PM. */
#define KMEM_SEG(pos)   KERNEL_DS
#define KMEM_OFF(pos)   ((segext_t)((pos) - ((unsigned long)KERNEL_DS << 4)))
/* only positions inside that window decode to real kernel data; anything else
 * (e.g. ps packing another process's SS) would silently read the wrong bytes */
#define KMEM_VALID(pos, len) \
    ((pos) >= ((unsigned long)KERNEL_DS << 4) && \
     (pos) - ((unsigned long)KERNEL_DS << 4) + (len) <= 0x10000UL)
#else
#define KMEM_SEG(pos)   ((seg_t)((unsigned long)(pos) >> 4))
#define KMEM_OFF(pos)   ((segext_t)((pos) & 0x0F))
#define KMEM_VALID(pos, len)    1
#endif

static size_t kmem_read(struct inode *inode, struct file *filp, char *data, size_t len)
{
    seg_t sseg = KMEM_SEG(filp->f_pos);
    segext_t soff = KMEM_OFF(filp->f_pos);

    if (!KMEM_VALID(filp->f_pos, (unsigned long)len))
        return -EFAULT;
    fmemcpyb((byte_t *)data, current->t_regs.ds, (byte_t *)soff, sseg, (word_t) len);
    filp->f_pos += len;
    return len;
}

static size_t kmem_write(struct inode *inode, struct file *filp, char *data, size_t len)
{
    seg_t dseg = KMEM_SEG(filp->f_pos);
    segext_t doff = KMEM_OFF(filp->f_pos);

    if (!KMEM_VALID(filp->f_pos, (unsigned long)len))
        return -EFAULT;
    /* FIXME: very dangerous! */
    fmemcpyb((byte_t *)doff, dseg, (byte_t *)data, current->t_regs.ds, (word_t) len);
    filp->f_pos += len;
    return len;
}

static int kmem_ioctl(struct inode *inode, struct file *file, int cmd, char *arg)
{
    unsigned short retword;
    struct mem_usage mu;

    switch (cmd) {
    case MEM_GETTASK:
        retword = (unsigned)task;
        break;
    case MEM_GETMAXTASKS:
        retword = max_tasks;
        break;
    case MEM_GETCS:
        retword = KERNEL_CS;
        break;
    case MEM_GETDS:
        retword = KERNEL_DS;
        break;
    case MEM_GETFARTEXT:
        retword = (unsigned)((long)buffer_init >> 16);
        break;
    case MEM_GETUSAGE:
        mm_get_usage(&mu);
        memcpy_tofs(arg, &mu, sizeof(struct mem_usage));
        return 0;
    case MEM_GETHEAP:
        retword = (unsigned)&_heap_all;
        break;
    case MEM_GETJIFFADDR:
    case MEM_GETUPTIME:
        retword = (unsigned)&jiffies;
        break;
    case MEM_GETSEGALL:
        retword = (unsigned)&_seg_all;
        break;
        /* fall thru */
    default:
        return -EINVAL;
    }
    put_user(retword, arg);
    return 0;
}

static int memory_lseek(struct inode *inode, struct file *filp, loff_t offset, int origin)
{
    switch (origin) {
    case 1:
        offset += filp->f_pos;
        /* fall thru */
    case 0:
        if (offset >= 0)
            break;
    default:
        return -EINVAL;
    }
    filp->f_pos = offset;
    return 0;
}

/*
 * /dev/port code
 */
#if defined(CONFIG_CHAR_DEV_MEM_PORT_READ)
static size_t port_read(struct inode *inode, struct file *filp, char *data, size_t len)
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
#       define  port_read       NULL
#endif

#if defined(CONFIG_CHAR_DEV_MEM_PORT_WRITE)
static size_t port_write(struct inode *inode, struct file *filp, char *data, size_t len)
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
#       define  port_write      NULL
#endif

#if defined(CONFIG_CHAR_DEV_MEM_PORT_READ) || defined(CONFIG_CHAR_DEV_MEM_PORT_WRITE)
static int port_lseek(struct inode *inode, struct file *filp, loff_t offset, int origin)
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

static struct file_operations port_fops = {
    port_lseek,                 /* lseek */
    port_read,                  /* read */
    port_write,                 /* write */
    NULL,                       /* readdir */
    NULL,                       /* select */
    NULL,                       /* ioctl */
    NULL,                       /* open */
    NULL                        /* release */
};
#endif

static struct file_operations null_fops = {
    null_lseek,                 /* lseek */
    null_read,                  /* read */
    null_write,                 /* write */
    NULL,                       /* readdir */
    NULL,                       /* select */
    NULL,                       /* ioctl */
    NULL,                       /* open */
    NULL                        /* release */
};

static struct file_operations zero_fops = {
    memory_lseek,               /* lseek */
    zero_read,                  /* read */
    null_write,                 /* write */
    NULL,                       /* readdir */
    NULL,                       /* select */
    NULL,                       /* ioctl */
    NULL,                       /* open */
    NULL                        /* release */
};

static struct file_operations kmem_fops = {
    memory_lseek,               /* lseek */
    kmem_read,                  /* read */
    kmem_write,                 /* write */
    NULL,                       /* readdir */
    NULL,                       /* select */
    kmem_ioctl,                 /* ioctl */
    NULL,                       /* open */
    NULL                        /* release */
};

static struct file_operations *mdev_fops[] = {
    NULL,
    &kmem_fops,                 /* DEV_MEM_MINOR */
    &kmem_fops,                 /* DEV_KMEM_MINOR */
    &null_fops,                 /* DEV_NULL_MINOR */
#if defined(CONFIG_CHAR_DEV_MEM_PORT_READ) || defined(CONFIG_CHAR_DEV_MEM_PORT_WRITE)
    &port_fops,                 /* DEV_PORT_MINOR */
#else
    NULL,
#endif
    &zero_fops,                 /* DEV_ZERO_MINOR */
};

/*
 * memory device open multiplexor
 */
static int memory_open(struct inode *inode, struct file *filp)
{
    unsigned int minor;

    minor = MINOR(inode->i_rdev);
    if (minor > DEV_MAX_MINOR || !mdev_fops[minor])
        return -ENXIO;
    filp->f_op = mdev_fops[minor];
    return 0;
}

static struct file_operations memory_fops = {
    NULL,                       /* lseek */
    NULL,                       /* read */
    NULL,                       /* write */
    NULL,                       /* readdir */
    NULL,                       /* select */
    NULL,                       /* ioctl */
    memory_open,                /* open */
    NULL                        /* release */
};

void INITPROC mem_dev_init(void)
{
    register_chrdev(MEM_MAJOR, "mem", &memory_fops);
}
#endif
