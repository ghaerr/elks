/*
 * /dev/kmsg - kernel log message device
 *
 * Provides read access to the kernel ring buffer populated by printk().
 * Writing is not supported. The ring buffer is circular and silently
 * discards the oldest messages when full.
 *
 * Registered as minor 8 under MEM_MAJOR in the mem.c multiplexer.
 */

#include <linuxmt/config.h>

#ifdef CONFIG_CHAR_DEV_KMSG

#include <linuxmt/kernel.h>
#include <linuxmt/fs.h>
#include <linuxmt/errno.h>

static int kmsg_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static size_t kmsg_read(struct inode *inode, struct file *filp,
    char *data, size_t len)
{
    size_t count = 0;
    int ch;

    while (count < len) {
        ch = kmsg_read_char();
        if (ch < 0)
            break;
        put_user_char((unsigned char)ch, (void *)(data++));
        count++;
    }
    filp->f_pos += count;
    return count;
}

static int kmsg_lseek(struct inode *inode, struct file *filp,
    loff_t offset, int origin)
{
    return -EINVAL;
}

struct file_operations kmsg_fops = {
    kmsg_lseek,                 /* lseek */
    kmsg_read,                  /* read */
    NULL,                       /* write */
    NULL,                       /* readdir */
    NULL,                       /* select */
    NULL,                       /* ioctl */
    kmsg_open,                  /* open */
    NULL                        /* release */
};

#endif
