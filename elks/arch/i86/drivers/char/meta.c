/*
 * arch/i86/drivers/char/meta.c
 *
 * Copyright (C) 1999 by Alistair Riddoch
 *
 * ELKS meta driver for user space device drivers (UDDs)
 */

#include <linuxmt/config.h>

#ifdef CONFIG_DEV_META

#include <linuxmt/major.h>
#include <linuxmt/fs.h>
#include <linuxmt/errno.h>
#include <linuxmt/mm.h>
#include <linuxmt/udd.h>
#include <linuxmt/debug.h>

#define MAJOR_NR	MISC_MAJOR

#define METADISK

#include "../block/blk.h"

struct ud_driver drivers[MAX_UDD];

struct ud_request requests[MAX_UDR];

static struct ud_driver * get_driver(major)
int major;
{
	int i;

	for(i = 0; i < MAX_UDD; i++) {
		if (drivers[i].udd_major == major) {
			return &drivers[i];
		}
	}
	return NULL;
}

struct ud_request * new_request()
{
	int i;

	for(i = 0; i < MAX_UDR; i++) {
		if (requests[i].udr_status == 0) {
			requests[i].udr_status = 1;
			return &requests[i];
		}
	}
	panic("Out of requests\n");
	return NULL;
}

int post_request(driver, request)
struct ud_driver * driver;
struct ud_request * request;
{
	if (driver->udd_req != NULL) {
		printk("Waiting to post a request\n");
		sleep_on(&driver->udd_rwait);
	}
	if (driver->udd_req != NULL) {
		printk("Signalled, exiting\n");
		return 1;
	}
	driver->udd_req = request;
	return 0;
}


static void do_meta_request(device)
kdev_t device;
{
	int major = MAJOR(device);
	struct ud_driver * driver = get_driver(major);
	struct ud_request * udr;
	char * buff;
	struct request * req;

	printk("do_meta_request %d %x\n", major, blk_dev[major].current_request);
	if (NULL == driver) {
		end_request(0, req->rq_dev);
		return;
	}
	printk("1");
	while(1) {
		req = blk_dev[major].current_request;
		printk("2");
		if (!req || req->rq_dev < 0 || req->rq_sector == -1)
			return;
		printk("5");
		udr = new_request();
		udr->udr_type = UDR_BLK + req->rq_cmd;
		udr->udr_ptr = req->rq_sector;
		udr->udr_minor = MINOR(req->rq_dev);
		printk("6");
		post_request(driver, udr);
		printk("7");
		/* Should really check here whether we have a request */
		if (req->rq_cmd == WRITE) {
		/* Can't do this, copies to the wrong task */
/*			verified_memcpy_tofs(driver->udd_data, buff, BLOCK_SIZE); */
/* FIXME FIXME			fmemcpy(driver->udd_task->mm.dseg, driver->udd_data, get_ds(), buff, 1024); */
		}
		printk("8");
		/* Wake up the driver so it can deal with the request */
		wake_up(&driver->udd_wait);
		printk("request init: wake driver, sleeping\n");
		sleep_on(&udr->udr_wait);
		printk("request continue\n");
		/* REQUEST HAS BEEN RETURNED BY USER PROGRAM */
		/* request must be dealt with and ended */
		if (udr->udr_status == 1) {
			end_request(0, req->rq_dev);
			udr->udr_status = 0;
			continue;
		}
		udr->udr_status = 0;
		buff = req->rq_buffer;
		if (req->rq_cmd == READ) {
		/* Can't do this, copies from the wrong task */
/*			verified_memcpy_fromfs(buff, driver->udd_data, BLOCK_SIZE); */
/* FIXME FIXME	 		fmemcpy(get_ds(), buff, driver->udd_task->mm.dseg, driver->udd_data, 1024); */
		}
		end_request(1, req->rq_dev);
		wake_up(&udr->udr_wait);
	}
}

void ubd_ioctl() {}

int ubd_open(inode, filp)
struct inode * inode;
struct file * filp;
{
	printk("ubd_open\n");
	return 0;
}

int ubd_release(inode, filp)
struct inode * inode;
struct file * filp;
{
	printk("ubd_release\n");
	return 0;
}

static struct file_operations ubd_fops =
{
	NULL,		/* lseek */
	block_read,	/* read */
	block_write,	/* write */
	NULL,		/* readdir */
	NULL,		/* select */
	ubd_ioctl,	/* ioctl */
	ubd_open,	/* open */
	ubd_release,	/* release */
#ifdef BLOAT_FS
	NULL,
	NULL,
	NULL
#endif
};

void ucd_lseek() {}
void ucd_read() {}
void ucd_write() {}
void ucd_select() {}
void ucd_open() {}
void ucd_release() {}

static int ucd_ioctl(inode, filp, cmd, arg)
struct inode * inode;
struct file * filp;
int cmd;
char * arg;
{
	printk("ucd_ioctl\n");
	switch (cmd) {
		default:
			return -EINVAL;
	}
}

static struct file_operations ucd_fops =
{
	ucd_lseek,	/* lseek */
	ucd_read,	/* read */
	ucd_write,	/* write */
	NULL,		/* readdir */
	ucd_select,	/* select */
	ucd_ioctl,	/* ioctl */
	ucd_open,	/* open */
	ucd_release,	/* release */
#ifdef BLOAT_FS
	NULL,
	NULL,
	NULL
#endif
};                                          

static int meta_initialised = 0;

void meta_lseek() {}
void meta_read() {}
void meta_write() {}
void meta_select() {}

static int meta_ioctl(inode, filp, cmd, arg)
struct inode * inode;
struct file * filp;
int cmd;
char * arg;
{
	int minor = MINOR(inode->i_rdev);
	struct ud_driver * driver;
	int i;

	printk("meta_ioctl %d %x\n", cmd, inode->i_rdev);
	if (minor == 0) {
		if (cmd != META_CREAT) {
			return -EINVAL;
		}
		printk("registering new ");
		for(i = 0; i < MAX_UDD; i++) {
			if (drivers[i].udd_type == UDD_NONE) {
				minor = i + 1;
				break;
			}
		}
		if (minor == 0) {
			return -ENODEV;
		}
		driver = &drivers[i];
		verified_memcpy_fromfs(driver,arg,sizeof(struct ud_driver_trunc));
		if (driver->udd_type == UDD_CHR_DEV) {
			printk("char ");
			if (register_chrdev(driver->udd_major, "meta", &ucd_fops) < 0) {
				goto reg_err;
			}
		} else if (driver->udd_type == UDD_BLK_DEV) {
			if ((i = verfy_area(driver->udd_data, 1024)) != 0) {
				driver->udd_type = UDD_NONE;
				return i;
			}
			printk("block ");
			/* verify are here on buffer */
			if (register_blkdev(driver->udd_major, "meta", &ubd_fops) < 0) {
				goto reg_err;
			}
			blk_dev[driver->udd_major].request_fn = do_meta_request;
		}
		printk("device\n");
		driver->udd_task = current;
/*		driver->udd_rwait = NULL;	/* FIXME: Not valid */
		return minor;
reg_err:
		driver->udd_type = UDD_NONE;
		return -ENODEV;
	}

	driver=&drivers[minor - 1];
	if (driver->udd_task != current) {
		printk("task mismatch %x %x\n", driver->udd_task, current);
		return -EINVAL;
	}
	switch (cmd) {
		case META_POLL:
			printk("waiting for request\n");

/*			driver->udd_wait = NULL;	/* FIXME: not valid */

			driver->udd_req = NULL;
			wake_up(&driver->udd_rwait);
			interruptible_sleep_on(&driver->udd_wait);
			printk("Waking up\n");
			if (driver->udd_req != NULL) {
				verified_memcpy_tofs(arg, driver->udd_req, sizeof(struct ud_request));
				printk("here is the request\n");
				return 0;
			}
			printk("No request! up\n");
			return -EINTR;
			break;		
		case META_ACK:
			printk("acknowledge request\n");
			if (driver->udd_req == NULL) {
				printk("No request was pending!\n");
				return -EINVAL;
			}
			verified_memcpy_fromfs(driver->udd_req, arg, sizeof(struct ud_request_trunc));
			wake_up(&driver->udd_req->udr_wait);

/*			driver->udd_req->udr_wait = NULL; /* FIXME: Not valid */

			interruptible_sleep_on(&driver->udd_req->udr_wait);
			break;		
		default:
			printk("Bad ioctl\n");
			return -EINVAL;
	}
	return 0;
}

static int meta_open(inode, filp)
struct inode * inode;
struct file * filp;
{
	printk("meta_open\n");
	return 0;
}

static int meta_release(inode, filp)
struct inode * inode;
struct file * filp;
{
	printk("meta_release\n");
	/* Wake up any processes waiting for this driver */
	return 0;
}

static struct file_operations meta_chr_fops =
{
	NULL,		/* lseek */
	NULL,		/* read */
	NULL,		/* write */
	NULL,		/* readdir */
	NULL,		/* select */
	meta_ioctl,	/* ioctl */
	meta_open,	/* open */
	meta_release,	/* release */
#ifdef BLOAT_FS
	NULL,
	NULL,
	NULL
#endif
};

void meta_init()
{
	int i;

	printk("meta driver Copyright (C) 1999 Alistair Riddoch\n");
	if ((i = register_chrdev(MAJOR_NR, DEVICE_NAME, &meta_chr_fops)) == 0) {
		meta_initialised = 1;
	}
}

#endif /* CONFIG_DEV_META */
