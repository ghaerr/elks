/*
 * tcpdev driver for ELKS kernel
 * Copyright (C) 2001 Harry Kalogirou
 *
 * Used by the user-space tcp/ip stack to pass
 * in and out data to the kernel
 *
 */

//#define DEBUG
#include <linuxmt/kernel.h>
#include <linuxmt/types.h>
#include <linuxmt/config.h>
#include <linuxmt/sched.h>
#include <linuxmt/debug.h>
#include <linuxmt/errno.h>
#include <linuxmt/major.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/mm.h>
#include <linuxmt/tcpdev.h>
#include <linuxmt/debug.h>

#include <arch/segment.h>

#ifdef CONFIG_INET

unsigned char tdin_buf[TCPDEV_INBUFFERSIZE];	/* for reading tcpdev*/
unsigned char tdout_buf[TCPDEV_OUTBUFFERSIZE];	/* for writing tcpdev*/

short bufin_sem, bufout_sem;	/* Buffer semaphores */

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
    debug("TCPDEV: read( %p, %p, %p, %u )\n",inode,filp,data,len);
    while (tdout_tail == 0) {
	if (filp->f_flags & O_NONBLOCK)
	    return -EAGAIN;
	interruptible_sleep_on(&tcpdevq);
	if (current->signal)
	    return -ERESTARTSYS;
    }

    /*
     *  If the userspace tcpip stack requests less data than what is in the
     *  buffer it will lose data, so the tcpip stack should read BIG.
     */
    len = len < tdout_tail ? len : tdout_tail;
    debug("TCPDEV: read() mark 1 - len = %u\n",len);
    memcpy_tofs(data, tdout_buf, len);
    tdout_tail = 0;
    up(&bufout_sem);
    debug("TCPDEV: read() returning with bufout_sem = %d\n",bufout_sem);
    if (bufout_sem > 0)
	panic("bufout_sem tragedy");
    return (int) len;
}

int tcpdev_inetwrite(char *data, unsigned int len)
{
    debug("TCPDEV: inetwrite( %p, %u )\n",data,len);
    if (len > TCPDEV_OUTBUFFERSIZE) {
	debug_net("tcpdev_inetwrite: len too large\n");
	return -EINVAL;
    }
    debug("TCPDEV: inetwrite() mark 1.\n");

    /* Data already in tdout_buf buffer */
    tdout_tail = len;
    wake_up(&tcpdevq);
    debug("TCPDEV: inetwrite() returning\n");
    return 0;
}

void tcpdev_clear_data_avail(void)
{
    up(&bufin_sem);
    debug("TCPDEV: clear_data_avail()\n");
    if (bufin_sem > 0)
	panic("bufin_sem tragedy");
}

static size_t tcpdev_write(struct inode *inode, struct file *filp,
			char *data, size_t len)
{
    debug("TCPDEV: write( %p, %p, %p, %u )\n",inode,filp,data,len);
    if (len > TCPDEV_INBUFFERSIZE) {
	debug_net("tcpdev_write len too large\n");
	return -EINVAL;
    }
    if (len > 0) {
	down(&bufin_sem);

	tdin_tail = (unsigned) len;
	memcpy_fromfs(tdin_buf, data, len);

	/* Call the af_inet code to handle the data */
	inet_process_tcpdev((char *)tdin_buf, len);
    }
    debug("TCPDEV: write() returning %u\n", len);
    return len;
}

static int tcpdev_select(struct inode *inode, struct file *filp, int sel_type)
{
    int ret = 0;

    debug("TCPDEV: select( %p, %p, %d )\n",inode,filp,sel_type);
    switch (sel_type) {
    case SEL_OUT:
	debug("TCPDEV: select() chose SEL_OUT\n");
	if (bufin_sem == 0)
	    ret = 1;
	else
	    select_wait(&tcpdevq);
	break;
    case SEL_IN:
	debug("TCPDEV: select() chose SEL_IN\n");
	if (tdout_tail != 0)
	    ret = 1;
	else
	    select_wait(&tcpdevq);
	break;
    default:
	debug("TCPDEV: select() chose unknown option %d.\n",sel_type);
    }
    debug("TCPDEV: select() returning %d\n",ret);
    return ret;
}

static int tcpdev_open(struct inode *inode, struct file *file)
{
    debug_net("TCPDEV: open(%p, %p) tcpdev_inuse %d\n",inode,file, tcpdev_inuse);
    if (!suser()) {
	debug_net("TCPDEV: open() returning -EPERM\n");
    	return -EPERM;
    }
    if (tcpdev_inuse) {
	debug_net("TCPDEV: open() returning -EBUSY\n");
	return -EBUSY;
    }
    debug_net("TCP open OK\n");
    tdin_tail = tdout_tail = 0;
    tcpdev_inuse = 1;
    return 0;
}

static void tcpdev_release(struct inode *inode, struct file *file)
{
    debug_net("TCPDEV: release(%p, %p) tcpdev_inuse %d\n",inode,file,tcpdev_inuse);
    tcpdev_inuse = 0;
}

/*@-type@*/

static struct file_operations tcpdev_fops = {
    NULL,			/* lseek */
    tcpdev_read,		/* read */
    tcpdev_write,		/* write */
    NULL,			/* readdir */
    tcpdev_select,		/* select */
    NULL,			/* ioctl */
    tcpdev_open,		/* open */
    tcpdev_release		/* release */
#ifdef BLOAT_FS
	,
    NULL,			/* fsync */
    NULL,			/* check_media_type */
    NULL			/* revalidate */
#endif
};

/*@+type@*/

void tcpdev_init(void)
{
    register_chrdev(TCPDEV_MAJOR, "tcpdev", &tcpdev_fops);

    debug_net("TCPDEV: init()\n");
    bufin_sem = bufout_sem = 0;
    tcpdev_inuse = 0;
}

#endif
