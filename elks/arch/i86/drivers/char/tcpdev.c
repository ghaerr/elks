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

unsigned char tdin_buf[TCPDEV_INBUFFERSIZE];
unsigned char tdout_buf[TCPDEV_OUTBUFFERSIZE];

short bufin_sem, bufout_sem;	/* Buffer semaphores */

static unsigned int tdin_tail, tdout_tail;

static struct wait_queue tcpdevq;

char tcpdev_inuse;

static size_t tcpdev_read(struct inode *inode, struct file *filp, char *data,
		       unsigned int len)
{
    debug4("TCPDEV: read( %p, %p, %p, %u )\n",inode,filp,data,len);
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
    debug1("TCPDEV: read() mark 1 - len = %u\n",len);
    memcpy_tofs(data, tdout_buf, len);
    tdout_tail = 0;
    up(&bufout_sem);
    debug1("TCPDEV: read() returning with bufout_sem = %d\n",bufout_sem);
    if (bufout_sem > 0)
	panic("bufout_sem tragedy");
    return (int) len;
}

int tcpdev_inetwrite(char *data, unsigned int len)
{
    debug2("TCPDEV: inetwrite( %p, %u )\n",data,len);
    if (len > TCPDEV_OUTBUFFERSIZE)
	return -EINVAL;		/* FIXME: make sure this never happens */
    debug("TCPDEV: inetwrite() mark 1.\n");
    down(&bufout_sem);

    /* Copy the data to the buffer */
/*    fmemcpy(kernel_ds, (__u16) tdout_buf, kernel_ds, (__u16) data, (__u16) len);*/
    memcpy(tdout_buf, data, len);
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
    debug4("TCPDEV: write( %p, %p, %p, %u )\n",inode,filp,data,len);
    if(len > 0) {
	down(&bufin_sem);

	tdin_tail = (unsigned) len;
	memcpy_fromfs(tdin_buf, data, len);

	/* Call the af_inet code to handle the data */
	inet_process_tcpdev(tdin_buf, len);
    }
    debug1("TCPDEV: write() returning %u\n", len);
    return len;
}

int tcpdev_select(struct inode *inode, struct file *filp, int sel_type)
{
    register char *ret = (char *)0;

    debug3("TCPDEV: select( %p, %p, %d )\n",inode,filp,sel_type);
    switch (sel_type) {
    case SEL_OUT:
	debug("TCPDEV: select() chose SEL_OUT\n");
	if (bufin_sem == 0)
	    ret = (char *)1;
	else
	    select_wait(&tcpdevq);
	break;
    case SEL_IN:
	debug("TCPDEV: select() chose SEL_IN\n");
	if (tdout_tail != 0)
	    ret = (char *)1;
	else
	    select_wait(&tcpdevq);
	break;
    default:
	debug1("TCPDEV: select() chose unknown option %d.\n",sel_type);
	break;
    }
    debug1("TCPDEV: select() returning %d\n",ret);
    return (int)ret;
}

static int tcpdev_open(struct inode *inode, struct file *file)
{
    debug2("TCPDEV: open( %p, %p )\n",inode,file);
    if(!suser()) {
	debug("TCPDEV: open() returning -EPERM\n");
    	return -EPERM;
    }
    if (tcpdev_inuse) {
	debug("TCPDEV: open() returning -EBUSY\n");
	return -EBUSY;
    }
    tdin_tail = tdout_tail = 0;
    tcpdev_inuse = 1;
    debug("TCPDEV: open() returning 0\n");
    return 0;
}

void tcpdev_release(struct inode *inode, struct file *file)
{
    debug2("TCPDEV: release( %p, %p )\n",inode,file);
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

    debug("TCPDEV: init()\n");
    bufin_sem = bufout_sem = 0;
    tcpdev_inuse = 0;
}

#endif
