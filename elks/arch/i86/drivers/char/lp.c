/*
 * lp driver for ELKS kernel
 * Copyright (C) 1997 Blaz Antonic
 * Debugged by Patrick Lam
 */

#include <linuxmt/types.h>
#include <linuxmt/config.h>
#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/major.h>

#include <linuxmt/lp.h>

/* This enables use of ports, detected by BIOS */
/* Works fine here and should be tested on other machines */
/* like 5150 as they are the oldest and might not support this */

#define BIOS_PORTS

#ifdef BIOS_PORTS
#define LP_PORTS 4
#endif

/* static int access_count[LP_PORTS] = {0,}; */

struct lp_info
{
	unsigned short io;
	char irq;
	char flags;
};

#ifdef BIOS_PORTS
/* We'll get port info from BIOS */ 
/* There are max 4 ports */
static struct lp_info ports[LP_PORTS] = 
{
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0
};
#else
/* extended to support some unusual ports;
 won't be used if BIOS_PORTS works fine */
 
static struct lp_info ports[LP_PORTS] =
{
	0x3BC,0,0,
	0x378,0,0,
	0x278,0,0,
	0x260,0,0,
	0x2E0,0,0,
	0x2E8,0,0,
	0x2F0,0,0,
	0x2F8,0,0,
	0x3E0,0,0,
	0x3E8,0,0
};
#endif

#ifndef BIOS_PORTS
static int port_order[LP_PORTS] = {0,};
#endif

static int lp_write();
static int lp_open();
static void lp_release();

static struct file_operations lp_fops = 
{
	NULL,		/* lseek */
	NULL,		/* read */
	lp_write,	/* write */
	NULL,		/* readdir */
	NULL,		/* select */
	NULL,		/* ioctl */
	lp_open,	/* open */
	lp_release,	/* release */
#ifdef BLOAT_FS
	NULL,		/* fsync */
	NULL,		/* check_media_type */
	NULL		/* revalidate */
#endif
};

int lp_char_polled(c, target)
char c;
int target;
{
	int status, wait;
	register struct lp_info * lpp;

#ifndef BIOS_PORTS
	target = port_order[target];
#endif
	lpp = &ports[target];
	/* safety checks */
	status = inb_p(lpp->io + STATUS);

	if (status == LP_OUTOFPAPER) {/* printer of out paper */
		printk("lp%d: out of paper\n", target);
		return 0;
	}
	if (!(status & LP_SELECTED)) {/* printer offline */
		printk("lp%d: printer offline\n", target);
		return 0;
	}

	while (!(status & LP_SELECTED)) {/* while not ready do */
		status = inb_p(lpp->io + STATUS);
		schedule(); /* Al: is it OK for this to be here ? */
	}

	/* send character to port */
	outb_p(c, lpp->io);

	/* strobe high */
	outb_p(LP_INIT | LP_SELECT | LP_STROBE, 
		lpp->io + CONTROL);

	/* 5 us delay */
	wait = 0;
	while (wait != LP_WAIT) wait++;

	/* strobe low */
	outb_p(LP_INIT | LP_SELECT, lpp->io + CONTROL);
	/* 5 us delay */
	while (wait) wait--;

	return 1;
}

int lp_write(inode, file, buf, count)
struct inode * inode;
struct file * file;
char * buf;
int count;
{
	int chrs = 0;

	while (chrs < count) {
		if (!lp_char_polled(peekb(current->t_regs.ds, buf + chrs), 
			MINOR(inode->i_rdev))) {
				break;
		}
		chrs++;
	}

	return chrs;
}

int lp_open(inode, file)
struct inode * inode;
struct file * file;
{
	int target;
	int status;
	register struct lp_info * lpp;
	target = MINOR(inode->i_rdev);
#ifndef BIOS_PORTS
	target = port_order[target];
#endif
	lpp = &ports[target];

	status = lpp->flags;

	if (!(status & LP_EXIST)) { /* if LP_EXIST flag not set */
/*		printk("lp: device lp%d doesn't exist\n", target); */
		return -ENODEV;
	}

	if (status & LP_BUSY) { /* if LP_BUSY flag set */
/*		printk("lp: device lp%d busy\n", target); */
		return -EBUSY;
	}

/*	inexistent port can't be busy */
	lpp->flags = LP_EXIST | LP_BUSY;

/*	access_count[port_order[target]]++; */

	return 0;
}

void lp_release(inode, file)
struct inode * inode;
struct file * file;
{
	int target;
	target = MINOR(inode->i_rdev);
#ifndef BIOS_PORTS
	target = port_order[target];
#endif

	ports[target].flags = LP_EXIST; /* not busy */

/*	access_count[port_order[target]]--; */

	return;
}

#if 0
int lp_write_polled(target, string, count)
int target;
char *string;
int count;
{
/*	int status, target;
	char * string; 
	int count; */

	/* send chars one by one */

}
#endif

#ifndef BIOS_PORTS
int lp_probe(lp)
register struct lp_info *lp;
{
	int wait;

	/* send 0 to port */
	outb_p(0, lp->io);

	/* 5 us delay */
	while (wait != LP_WAIT) wait++;

	/* 0 expected; 255 returned if inexistent port */
	if (inb_p(lp->io) == 0) {
		lp->flags = LP_EXIST;
		return 0;
	}
	return -1;
}
#endif

void lp_init()
{
	register struct lp_info *lp = &ports[0];
	int i;
	int count = 0;

#ifdef BIOS_PORTS
/* only ports 0, 1, 2 and 3 may exist according to RB's intlist */
	for (i = 0; i < LP_PORTS; i++)
	{
	/* 8 is offset for LPT info, 2 bytes for each entry */
		lp->io = peekw(0x40, 8 + 2*i);
	/* returns 0 if port wasn't detected by BIOS at bootup */
		if (lp->io != 0) {
			printk("lp%d at 0x%x, using polling driver\n",
				i, lp->io);
			lp->flags = LP_EXIST;
			lp++;
			count++;
		}
		else
		/* there can be no more ports */
			break;
	}
#else
	/* probe for ports */
	for (i = 0; i < LP_PORTS; i++) {
		if (lp_probe(lp) == 0) {
			printk("lp%d at 0x%x, using polling driver\n",
				i, lp->io);
			port_order[count] = i;
			count++;
		}
		lp++;
	}
#endif

	if (count == 0) {
		printk("lp: no ports found\n");
	}

	/* register device */
	i = register_chrdev(LP_MAJOR, LP_DEVICE_NAME, &lp_fops);

	if (i != 0) {
		printk("lp: unable to register\n");
	}
}
