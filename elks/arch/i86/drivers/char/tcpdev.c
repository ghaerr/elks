/*
 * tcpdev driver for ELKS kernel
 * Copyright (C) 2001 Harry Kalogirou
 *
 * Used by the user-space tcp/ip stack to pass
 * in and out data to the kernel
 *
 */

#include <linuxmt/types.h>
#include <linuxmt/config.h>
#include <linuxmt/sched.h>
#include <linuxmt/debug.h>
#include <linuxmt/errno.h>
#include <linuxmt/major.h>
#include <linuxmt/fcntl.h>
#if NEED_RESCHED
#include <linuxmt/sched.h>
#endif
#include <linuxmt/tcpdev.h>

#ifdef CONFIG_INET

extern inet_process_tcpdev();

unsigned char td_buf[TCPDEV_WRITEBUFFERSIZE];
static unsigned char buf_lock;
static unsigned int buf_tail;
static unsigned char buf_data_avail;

static struct wait_queue tcpdev_wait;
static struct wait_queue tcpdevp;

static int tcpdev_read(inode, filp, data, len)
struct inode *inode;
struct file *filp;
char *data;
int len;
{
	printd_td("tcpdev : read()\n");
	while(buf_data_avail == 0){
		if(filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
			
		interruptible_sleep_on(tcpdevp);
		if(current->signal)
			return -ERESTARTSYS;
	}
	
	/*
	 * If the userspace tcpip stack requests less data
	 * than what is in the buffer is will loose data
	 * so the tcpip stack sould read BIG
	 */
	len = len < buf_tail ? len : buf_tail;
	memcpy_tofs(data, td_buf, len);
	
	buf_tail = buf_data_avail = 0;
	buf_lock = 0;
	
	wake_up(&tcpdev_wait);

    return len;
}

int tcpdev_inetwrite(data, len)
char *data;
int len;
{
	unsigned int ds;

	printd_td("tcpdev : inetwrite()\n");	
	if(len > TCPDEV_WRITEBUFFERSIZE)
		return -EINVAL;

	while(buf_lock == 1){	
		interruptible_sleep_on(&tcpdev_wait);
		if(current->signal)
			return -ERESTARTSYS;
	}

	/* Lock the buffer */
	buf_lock = 1;
	
	/* Copy the data to the buffer */
	ds = get_ds();
	fmemcpy(ds, td_buf, ds, data, len);
	buf_tail = len;
	buf_data_avail = 1;
	
	wake_up(&tcpdevp);
	/* The buffer will be unlocked when the data are read */
/*	
	while(buf_lock == 1){	
		interruptible_sleep_on(&tcpdev_wait);
	}	
	*/
	return 0;
}

void tcpdev_clear_data_avail(){
	buf_lock = 0;
}

static int tcpdev_write(inode, filp, data, len)
struct inode *inode;
struct file *filp;
char *data;
int len;
{
	int ret;

	printd_td("tcpdev : write()\n");	
	if(len <= 0)
		return 0;
		
	if(len > TCPDEV_WRITEBUFFERSIZE)
		return -EINVAL;
		
	while(buf_lock == 1){
		if(filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
			
		interruptible_sleep_on(&tcpdev_wait);
		if(current->signal)
			return -ERESTARTSYS;
	}
	
	/* Cleared to write. Lock the buffer */
	buf_lock = 1;
	
	buf_tail = len;
	memcpy_fromfs(td_buf, data, len);
	
	/* Call the af_inet code to handle the data */
	inet_process_tcpdev(td_buf, len);
	
	buf_lock = 0;
/*	buf_data_avail = 0;*/
	return len;
	
}

int tcpdev_select(inode, filp, sel_type)
struct inode *inode;
struct file *filp;
int sel_type;
{
    switch (sel_type) {
    case SEL_OUT:
    	if(buf_lock == 0)
    		return 1;
		select_wait(&tcpdevp);
		return 0;
	case SEL_IN:
		if(buf_data_avail == 1)
			return 1;
		select_wait(&tcpdevp);
		return 0;
    }
    return 0;
}

static int tcpdev_open(inode, file)
struct inode * inode;
struct file * file;
{
	printd_td("tcpdev : open()\n");	
	return 0;
}

static struct file_operations tcpdev_fops = 
{
	NULL,		/* lseek */
	tcpdev_read,	/* read */
	tcpdev_write,	/* write */
	NULL,		/* readdir */
	tcpdev_select,		/* select */
	NULL,		/* ioctl */
	tcpdev_open,	/* open */
	NULL,	/* release */
#ifdef BLOAT_FS
	NULL,		/* fsync */
	NULL,		/* check_media_type */
	NULL		/* revalidate */
#endif
};

void tcpdev_init()
{
    register_chrdev(TCPDEV_MAJOR, "tcpdev", &tcpdev_fops);

    buf_lock = 0;
    buf_data_avail = 0;
}

#endif

