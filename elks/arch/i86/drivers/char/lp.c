/*
 * lp driver for ELKS kernel
 * Copyright (C) 1997 Blaz Antonic
 * Debugged by Patrick Lam
 */

#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/kernel.h>
#include <linuxmt/lp.h>
#include <linuxmt/mm.h>
#include <linuxmt/major.h>
#include <linuxmt/sched.h>
#include <linuxmt/types.h>
#include <linuxmt/debug.h>

#include <arch/io.h>

/* byte-wide put_user() */
#define put_user(offset) peekb(current->t_regs.ds, offset)

#if 0
static int access_count[LP_PORTS] = {0,};
#endif

struct lp_info {
    unsigned short io;
    char irq;
    char flags;
};

#ifdef BIOS_PORTS

/*@-type@*/

/* We'll get port info from BIOS. There are 4 ports max. */
static struct lp_info ports[LP_PORTS] = { 0, 0, 0, 0 };

/*@+type@*/

#else

/* extended to support some unusual ports; not used if BIOS_PORTS works */
static struct lp_info ports[LP_PORTS] = {
    0x3BC, 0, 0,
    0x378, 0, 0,
    0x278, 0, 0,
    0x260, 0, 0,
    0x2E0, 0, 0,
    0x2E8, 0, 0,
    0x2F0, 0, 0,
    0x2F8, 0, 0,
    0x3E0, 0, 0,
    0x3E8, 0, 0
};

#endif

#ifndef BIOS_PORTS
static int port_order[LP_PORTS] = { 0, };
#endif

int lp_reset(int target)
{
    struct lp_info *lpp;
    int tmp;

#ifndef BIOS_PORTS
    target = port_order[target];
#endif

    lpp = &ports[target];

    LP_CONTROL(LP_SELECT, lpp);
    tmp = 0;
    while (tmp != LP_RESET_WAIT)
	tmp++;
    LP_CONTROL(LP_SELECT | LP_INIT, lpp);

    /* we are (ab)using the same variable, no reason to waste DS */
    tmp = LP_STATUS(lpp);
    return tmp;
}

int lp_char_polled(char c, unsigned int target)
{
    int status, wait;
    register struct lp_info *lpp;

#ifndef BIOS_PORTS
    target = port_order[target];
#endif

    lpp = &ports[target];

    /* safety checks */
    status = LP_STATUS(lpp);

    /* safety checks, note that LP_ERROR and LP_SELECTED are inverted signals */

    if (status & LP_OUTOFPAPER) {	/* printer out of paper */
	printk("lp%d: out of paper\n", target);
	return 0;
    }

#if 0

    if (!(status & LP_SELECTED)) {	/* printer offline */
	printk("lp%d: printer offline\n", target);
	return 0;
    }

#else

    wait = 0;

    while (!(status & LP_SELECTED) && (wait < LP_CHAR_WAIT))
	/* while not ready do busy loop */
    {
	status = LP_STATUS(lpp);
	wait++;

#if NEED_RESCHED
	if (need_resched)
	    schedule();
#endif

    }
    if (wait == LP_CHAR_WAIT) {
	printk("lp%d: timed out\n", target);
	return 0;
    }

#endif

    if (!(status & LP_ERROR)) {	/* printer error */
	printk("lp%d: printer error\n", target);
	return 0;
    }

    /* send character to port */
    outb_p((unsigned char) c, (void *) lpp->io);

    /* 5 us delay */
    wait = 0;
    while (wait++ != LP_STROBE_WAIT)
	/* Do nothing */ ;

    /* strobe high */
    LP_CONTROL(LP_SELECT | LP_INIT | LP_STROBE, lpp);

    /* strobe low */
    LP_CONTROL(LP_SELECT | LP_INIT, lpp);

    /* 5 us delay */
    while (wait--)
	/* Do nothing */ ;

    return 1;
}

int lp_write(struct inode *inode, struct file *file, char *buf, int count)
{
    int chrs = 0, tmp;

#if 0
    /* initialize printer */
    lp_reset(MINOR(inode->i_rdev));
#endif

    while (chrs < count) {
	tmp = put_user((__u16) (buf + chrs));
	if (!lp_char_polled(tmp, MINOR(inode->i_rdev)))
	    break;
	chrs++;
    }
    return chrs;
}

int lp_open(struct inode *inode, struct file *file)
{
    register struct lp_info *lpp;
    unsigned short int target;
    short int status;

    target = MINOR(inode->i_rdev);

#ifndef BIOS_PORTS
    target = port_order[target];
#endif

    lpp = &ports[target];

    status = lpp->flags;

    if (!(status & LP_EXIST)) {	/* if LP_EXIST flag not set */
	debug1("lp: device lp%d doesn't exist\n", target);
	return -ENODEV;
    }

    if (status & LP_BUSY) {	/* if LP_BUSY flag set */
	debug1("lp: device lp%d busy\n", target);
	return -EBUSY;
    }

/*
 * nonexistent port can't be busy
 */
    lpp->flags = LP_EXIST | LP_BUSY;

#if 0
    access_count[port_order[target]]++;
#endif

    return 0;
}

void lp_release(struct inode *inode, struct file *file)
{
    unsigned short int target = MINOR(inode->i_rdev);

#ifndef BIOS_PORTS
    target = port_order[target];
#endif

    ports[target].flags = LP_EXIST;	/* not busy */

#if 0
    access_count[port_order[target]]--;
#endif

    return;
}

#ifndef BIOS_PORTS

int lp_probe(register struct lp_info *lp)
{
    int wait;

    /* send 0 to port */
    outb_p((unsigned char) (LP_DUMMY), (void *) lp->io);

    /* 5 us delay */
    while (wait != LP_WAIT)
	wait++;

    /* 0 expected; 255 returned if nonexistent port */
    if (inb_p(lp->io) == LP_DUMMY) {
	lp->flags = LP_EXIST;
	return 0;
    }
    return -1;
}

#endif

/*@-type@*/

static struct file_operations lp_fops = {
    NULL,			/* lseek */
    NULL,			/* read */
    lp_write,			/* write */
    NULL,			/* readdir */
    NULL,			/* select */
    NULL,			/* ioctl */
    lp_open,			/* open */
    lp_release,			/* release */
#ifdef BLOAT_FS
    NULL,			/* fsync */
    NULL,			/* check_media_type */
    NULL			/* revalidate */
#endif
};

/*@+type@*/

void lp_init(void)
{
    register struct lp_info *lp = &ports[0];
    int i;
    int count = 0;

#ifdef BIOS_PORTS

    /* only ports 0, 1, 2 and 3 may exist according to RB's intlist */
    for (i = 0; i < LP_PORTS; i++) {
	/* 8 is offset for LPT info, 2 bytes for each entry */
	lp->io = peekw(0x40, (__u16) (2 * i + 8));
	/* returns 0 if port wasn't detected by BIOS at bootup */
	if (lp->io) {
	    printk("lp%d at 0x%x, using polling driver\n", i, lp->io);
	    lp->flags = LP_EXIST;
	    lp++;
	    count++;
	} else
	    /* there can be no more ports */
	    break;
    }

#else

    /* probe for ports */
    for (i = 0; i < LP_PORTS; i++) {
	if (!lp_probe(lp)) {
	    printk("lp%d at 0x%x, using polling driver\n", i, lp->io);
	    port_order[count] = i;
	    count++;
	}
	lp++;
    }

#endif

    if (count == 0)
	printk("lp: no ports found\n");

    /* register device */
    i = register_chrdev(LP_MAJOR, LP_DEVICE_NAME, &lp_fops);

    if (i != 0)
	printk("lp: unable to register\n");
}
