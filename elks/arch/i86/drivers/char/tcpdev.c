/*
 * tcpdev driver for ELKS kernel
 * Copyright (C) 2001 Harry Kalogirou
 *
 * Used by the user-space tcp/ip stack to pass
 * in and out data to the kernel
 *
 */

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

#include <arch/segment.h>

#ifdef CONFIG_INET

extern inet_process_tcpdev();

unsigned char tdin_buf[TCPDEV_INBUFFERSIZE];
unsigned char tdout_buf[TCPDEV_OUTBUFFERSIZE];

short bufin_sem, bufout_sem;	/* Buffer semaphores */

static unsigned int tdin_tail, tdout_tail;

static struct wait_queue tcpdevq;

static char tcpdev_inuse;

static int tcpdev_read(struct inode *inode, struct file *filp, char *data,
		       unsigned int len)
{
    debug1("TCPDEV: read called with room for %d bytes of data.\n",len);
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
    memcpy_tofs(data, tdout_buf, len);
    tdout_tail = 0;
    up(&bufout_sem);
    if (bufout_sem > 0)
	panic("bufout_sem tragedy");
    return (int) len;
}

int tcpdev_inetwrite(char *data, unsigned int len)
{
    __u16 ds;

    debug1("TCPDEV: inetwrite called with %d bytes of data.\n", len);
    if (len > TCPDEV_OUTBUFFERSIZE)
	return -EINVAL;		/* FIXME: make sure this never happens */
    down(&bufout_sem);
    debug("TCPDEV: inetwrite() writing.\n");

    /* Copy the data to the buffer */
    ds = get_ds();
    fmemcpy(ds, (__u16) tdout_buf, ds, (__u16) data, (__u16) len);
    tdout_tail = len;
    wake_up(&tcpdevq);
    return 0;
}

void tcpdev_clear_data_avail(void)
{
    up(&bufin_sem);
    if (bufin_sem > 0)
	panic("bufin_sem tragedy");
}

static int tcpdev_write(struct inode *inode, struct file *filp,
			char *data, size_t len)
{
    int ret;

    debug1("TCPDEV: write called with %d bytes of data.\n", len);
    if (len <= 0)
	ret = 0;
    else {
	down(&bufin_sem);

	tdin_tail = (unsigned) len;
	memcpy_fromfs(tdin_buf, data, len);

	/* Call the af_inet code to handle the data */
	inet_process_tcpdev(tdin_buf, len);

	ret = (int) len;
    }
    return ret;
}

int tcpdev_select(struct inode *inode, struct file *filp, int sel_type)
{
    switch (sel_type) {
    case SEL_OUT:
	if (bufin_sem == 0)
	    return 1;
	select_wait(&tcpdevq);
	return 0;
    case SEL_IN:
	if (tdout_tail != 0)
	    return 1;
	select_wait(&tcpdevq);
	return 0;
    }
    return 0;
}

static int tcpdev_open(struct inode *inode, struct file *file)
{
    debug("TCPDEV: open called.\n");

    if(!suser())
    	return -EPERM;

    if (tcpdev_inuse)
	return -EBUSY;

    tdin_tail = tdout_tail = 0;

    tcpdev_inuse = 1;
    return 0;
}

int tcpdev_release(struct inode *inode, struct file *file)
{
    debug("TCPDEV: release called.\n");

    tcpdev_inuse = 0;
    return 0;
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

    debug("TCPDEV: init called.\n");

    bufin_sem = bufout_sem = 0;
    tcpdev_inuse = 0;
}

#endif
