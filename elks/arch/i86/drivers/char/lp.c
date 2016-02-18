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

#if 0
static int access_count[LP_PORTS] = {0,};
#endif

struct lp_info {
    char *io;
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
    register struct lp_info *lpp;
    register char *tmpp;

#ifndef BIOS_PORTS
    target = port_order[target];
#endif

    lpp = &ports[target];

    LP_CONTROL(LP_SELECT, lpp);
    tmpp = 0;
    while (((int)tmpp) != LP_RESET_WAIT)
	tmpp++;
    LP_CONTROL(LP_SELECT | LP_INIT, lpp);

    return LP_STATUS(lpp);
}

int lp_char_polled(char c, unsigned int target)
{
    int wait;
    register struct lp_info *lpp;

#ifndef BIOS_PORTS
    target = port_order[target];
#endif

    lpp = &ports[target];

    {
	register char *statusp;

	/* safety checks */
	statusp = (char *) LP_STATUS(lpp);

	/* safety checks, note that LP_ERROR and LP_SELECTED
	 * are inverted signals */

	if (((int)statusp) & LP_OUTOFPAPER) { /* printer out of paper */
	    printk("lp%d: out of paper\n", target);
	    return 0;
	}

#if 0

	if (!(((int)statusp) & LP_SELECTED)) { /* printer offline */
	    printk("lp%d: printer offline\n", target);
	    return 0;
	}

#else

	wait = 0;

	while (!(((int)statusp) & LP_SELECTED) && (wait < LP_CHAR_WAIT))
	    /* while not ready do busy loop */
	    {
		statusp = (char *) LP_STATUS(lpp);
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

	if (!(((int)statusp) & LP_ERROR)) { /* printer error */
	    printk("lp%d: printer error\n", target);
	    return 0;
	}
    }

    /* send character to port */
    outb_p((unsigned char) c, lpp->io);

    {
	register char *waitp;
	/* 5 us delay */
	waitp = 0;
	while (((int)(waitp++)) != LP_STROBE_WAIT)
	    /* Do nothing */ ;

    /* strobe high */
	LP_CONTROL(LP_SELECT | LP_INIT | LP_STROBE, lpp);

	/* strobe low */
	LP_CONTROL(LP_SELECT | LP_INIT, lpp);

	/* 5 us delay */
	while (waitp--)
	    /* Do nothing */ ;
    }

    return 1;
}

size_t lp_write(struct inode *inode, struct file *file, char *buf, int count)
{
    register char *chrsp;

#if 0

    /* initialize printer */
    lp_reset(MINOR(inode->i_rdev));

#endif

    chrsp = 0;
    while (((int)chrsp) < count) {
	if (!lp_char_polled((int)get_user_char((void *)(buf++)),
			    MINOR(inode->i_rdev)))
	    break;
	chrsp++;
    }
    return (size_t)chrsp;
}

int lp_open(struct inode *inode, struct file *file)
{
    register struct lp_info *lpp;
    register char *statusp;
    unsigned short int target;
#if 0
    short int status;
#endif

    target = MINOR(inode->i_rdev);

#ifndef BIOS_PORTS
    target = port_order[target];
#endif

    lpp = &ports[target];

    statusp = (char *)((short int)(lpp->flags));

    if (!(((short int)statusp) & LP_EXIST)) { /* if LP_EXIST flag not set */
	debug1("lp: device lp%d doesn't exist\n", target);
	return -ENODEV;
    }

    if (((short int)statusp) & LP_BUSY) { /* if LP_BUSY flag set */
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
    register char *waitp;

    /* send 0 to port */
    outb_p((unsigned char) (LP_DUMMY), lp->io);

    /* 5 us delay */
    while (((int)waitp) != LP_WAIT)
	waitp++;

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
    lp_release			/* release */
#ifdef BLOAT_FS
	,
    NULL,			/* fsync */
    NULL,			/* check_media_type */
    NULL			/* revalidate */
#endif
};

/*@+type@*/

void lp_init(void)
{
    register struct lp_info *lp = &ports[0];
    register char *ip;
    int count = 0;

#ifdef BIOS_PORTS

    /* only ports 0, 1, 2 and 3 may exist according to RB's intlist */
    for (ip = 0; ((int)ip) < LP_PORTS; ip++) {
	/* 8 is offset for LPT info, 2 bytes for each entry */
	lp->io = (char *)peekw(0x40, (__u16) (2 * ((int)ip) + 8));
	/* returns 0 if port wasn't detected by BIOS at bootup */
	if (!lp->io)
	    break;		/* there can be no more ports */
	printk("lp%d at 0x%x, using polling driver\n", (int)ip, lp->io);
	lp->flags = LP_EXIST;
	lp++;
    }
    count = (int)ip;

#else

    /* probe for ports */
    for (ip = 0; ((int)ip) < LP_PORTS; ip++) {
	if (!lp_probe(lp)) {
	    printk("lp%d at 0x%x, using polling driver\n", (int)ip, lp->io);
	    port_order[count] = (int)ip;
	    count++;
	}
	lp++;
    }

#endif

    if (count == 0)
	printk("lp: no ports found\n");

    /* register device */
    if (register_chrdev(LP_MAJOR, LP_DEVICE_NAME, &lp_fops))
	printk("lp: unable to register\n");
}
