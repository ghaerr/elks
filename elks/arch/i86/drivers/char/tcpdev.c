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

unsigned char tdin_buf[TCPDEV_INBUFFERSIZE];
unsigned char tdout_buf[TCPDEV_OUTBUFFERSIZE];
static unsigned int tdin_tail, tdout_tail;
short	bufin_sem, bufout_sem;	/* Buffer semaphores */

static struct wait_queue tcpdevp;

static char tcpdev_inuse;

static int tcpdev_read(inode, filp, data, len)
struct inode *inode;
struct file *filp;
char *data;
int len;
{
	printd_td("tcpdev : read()\n");
	
	while(tdout_tail == 0){
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
	len = len < tdout_tail ? len : tdout_tail;
	memcpy_tofs(data, tdout_buf, len);
	
	tdout_tail = 0;

	up(&bufout_sem);
	if(bufout_sem > 0)panic("bufout_sem tragedy");

    return len;
}

int tcpdev_inetwrite(data, len)
char *data;
int len;
{
	unsigned int ds;

	printd_td("tcpdev : inetwrite()\n");	
	if(len > TCPDEV_OUTBUFFERSIZE)
		return -EINVAL; /* FIXME: make sure this never happen */
		
	down(&bufout_sem);
	printd_td("tcpdev : inetwrite() writing\n");
	
	/* Copy the data to the buffer */
	ds = get_ds();
	fmemcpy(ds, tdout_buf, ds, data, len);
	tdout_tail = len;
	
	wake_up(&tcpdevp);

	return 0;
}

void tcpdev_clear_data_avail(){
	up(&bufin_sem);
	if(bufin_sem > 0)panic("bufin_sem tragedy");
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
		
	down(&bufin_sem);

	tdin_tail = len;
	memcpy_fromfs(tdin_buf, data, len);
	
	/* Call the af_inet code to handle the data */
	inet_process_tcpdev(tdin_buf, len);

	return len;	
}

int tcpdev_select(inode, filp, sel_type)
struct inode *inode;
struct file *filp;
int sel_type;
{
    switch (sel_type) {
    case SEL_OUT:
		if(bufin_sem == 0)
    		return 1;
		select_wait(&tcpdevp);
		return 0;
	case SEL_IN:
		if(tdout_tail != 0){
			return 1;
		}
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
	
	if(tcpdev_inuse)
		return -EBUSY;
		
    tdin_tail = tdout_tail = 0;
	
	tcpdev_inuse = 1;
	return 0;
}

int tcpdev_release(inode,file)
struct inode *inode;
struct file *file;
{
	tcpdev_inuse = 0;
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
	tcpdev_release,	/* release */
#ifdef BLOAT_FS
	NULL,		/* fsync */
	NULL,		/* check_media_type */
	NULL		/* revalidate */
#endif
};

void tcpdev_init()
{
    register_chrdev(TCPDEV_MAJOR, "tcpdev", &tcpdev_fops);

	bufin_sem = bufout_sem = 0;
    tcpdev_inuse = 0;
}

#endif

