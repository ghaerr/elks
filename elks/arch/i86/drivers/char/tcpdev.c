/*
 * tcpdev driver for ELKS kernel
 * Copyright (C) 2001 Harry Kalogirou
 *
 * Used by the user-space tcp/ip stack to pass
 * in and out data to the kernel
 *
 */

//#define DEBUG 1
#include <linuxmt/config.h>
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/major.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/mm.h>
#include <linuxmt/tcpdev.h>
#include <linuxmt/debug.h>

#include <arch/segment.h>

#ifdef CONFIG_INET

unsigned char tdin_buf[TCPDEV_INBUFFERSIZE];    /* for reading tcpdev*/
unsigned char tdout_buf[TCPDEV_OUTBUFFERSIZE];  /* for writing tcpdev*/

sem_t bufin_sem, bufout_sem;    /* Buffer semaphores */

static unsigned int tdin_tail, tdout_tail;

static struct wait_queue tcpdevq;

char tcpdev_inuse;

char *get_tdout_buf(void)
{
    down(&bufout_sem);
    return (char *)tdout_buf;
}

static size_t tcpdev_read(struct inode *inode, struct file *filp, char *data,
                       unsigned int len)
{
    debug("TCPDEV(%P) read %u\n", len);

    while (tdout_tail == 0) {
        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;
        interruptible_sleep_on(&tcpdevq);
        if (current->signal) {
            debug_net("TCPDEV(%P) read RESTARTSYS tdtail %d\n", tdout_tail);
            return -ERESTARTSYS;
        }
    }

    /*
     *  If the userspace tcpip stack requests less data than what is in the
     *  buffer it will lose data, so the tcpip stack should read BIG.
     */
    if (len < tdout_tail)
        debug_net("TCPDEV(%P) read len too small %u\n", len);
    else len = tdout_tail;
    memcpy_tofs(data, tdout_buf, len);
    tdout_tail = 0;
    up(&bufout_sem);

    debug("TCPDEV(%P) read retval %u bufout_sem %d\n", len, bufout_sem);
    //if (bufout_sem > 0)
        //panic("bufout_sem tragedy");
    return len;
}

int tcpdev_inetwrite(void *data, unsigned int len)
{
    debug("TCPDEV(%P) inetwrite %u\n", len);
    if (len > TCPDEV_OUTBUFFERSIZE) {
        debug_net("TCPDEV(%P) inetwrite len too large %u\n", len);
        return -EINVAL;
    }

    /* Data already in tdout_buf buffer */
    tdout_tail = len;
    wake_up(&tcpdevq);
    return 0;
}

void tcpdev_clear_data_avail(void)
{
    up(&bufin_sem);
}

static size_t tcpdev_write(struct inode *inode, struct file *filp,
                        char *data, size_t len)
{
    debug("TCPDEV(%P) write %u\n", len);
    if (len > TCPDEV_INBUFFERSIZE) {
        debug_net("TCPDEV(%P) write len too large %u\n", len);
        return -EINVAL;
    }
    if (len > 0) {
        down(&bufin_sem);

        tdin_tail = (unsigned) len;
        memcpy_fromfs(tdin_buf, data, len);

        /* Call the af_inet code to handle the data */
        inet_process_tcpdev((char *)tdin_buf, len);
    }
    debug("TCPDEV(%P) write retval %u\n", len);
    return len;
}

static int tcpdev_select(struct inode *inode, struct file *filp, int sel_type)
{
    int ret = 0;

    switch (sel_type) {
    case SEL_OUT:
        debug("TCPDEV(%P) select SEL_OUT\n");
        if (bufin_sem == 0)
            ret = 1;
        else
            select_wait(&tcpdevq);
        break;
    case SEL_IN:
        debug("TCPDEV(%P) select SEL_IN\n");
        if (tdout_tail != 0)
            ret = 1;
        else
            select_wait(&tcpdevq);
        break;
    default:
        debug("TCPDEV(%P) select bad type %d\n", sel_type);
    }
    debug("TCPDEV(%P) select retval %d\n", ret);
    return ret;
}

static int tcpdev_open(struct inode *inode, struct file *file)
{
    debug_net("TCPDEV(%P) open inuse %d\n", tcpdev_inuse);
    if (!suser()) {
        debug_net("TCPDEV open retval -EPERM\n");
        return -EPERM;
    }
    if (tcpdev_inuse) {
        debug_net("TCPDEV open retval -EBUSY\n");
        return -EBUSY;
    }
    tdin_tail = tdout_tail = 0;
    tcpdev_inuse = 1;
    return 0;
}

static void tcpdev_release(struct inode *inode, struct file *file)
{
    debug_net("TCPDEV(%P) release inuse %d\n", tcpdev_inuse);
    tcpdev_inuse = 0;
}

static struct file_operations tcpdev_fops = {
    NULL,                       /* lseek */
    tcpdev_read,                /* read */
    tcpdev_write,               /* write */
    NULL,                       /* readdir */
    tcpdev_select,              /* select */
    NULL,                       /* ioctl */
    tcpdev_open,                /* open */
    tcpdev_release              /* release */
};

void INITPROC tcpdev_init(void)
{
    register_chrdev(TCPDEV_MAJOR, "tcpdev", &tcpdev_fops);
    bufin_sem = bufout_sem = 0;
    tcpdev_inuse = 0;
}

#endif
