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

/* We'll get port info from BIOS. There are 4 ports max. */
static struct lp_info ports[LP_PORTS];

#else

/* extended to support some unusual ports; not used if BIOS_PORTS works */
static struct lp_info ports[] = {
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

static int port_order[LP_PORTS] = { 0, };

#endif

/*
#define USE_LP_RESET
*/

#ifdef USE_LP_RESET
static int lp_reset(int target)
{
    register struct lp_info *lpp;
    register char *tmpp;

#ifdef BIOS_PORTS
    lpp = &ports[target];
#else
    lpp = &ports[port_order[target]];
#endif

    LP_CONTROL(LP_SELECT, lpp);
    tmpp = (char *)LP_RESET_WAIT;
    do {
    } while ((int)(--tmpp));
    LP_CONTROL(LP_SELECT | LP_INIT, lpp);

    return LP_STATUS(lpp);
}
#endif

static int lp_char_polled(char c, unsigned int target)
{
    int wait;
    register struct lp_info *lpp;

#ifdef BIOS_PORTS
    lpp = &ports[target];
#else
    lpp = &ports[port_order[target]];
#endif

    {
	int status;

	/* safety checks */
	status = LP_STATUS(lpp);

	/* safety checks, note that LP_ERROR and LP_SELECTED
	 * are inverted signals */

	if (status & LP_OUTOFPAPER) { /* printer out of paper */
	    printk("lp%d: out of paper\n", target);
	    return 0;
	}

#if 0

	if (!(status & LP_SELECTED)) { /* printer offline */
	    printk("lp%d: printer offline\n", target);
	    return 0;
	}

#else

	wait = LP_CHAR_WAIT + 1;

	while (!((status = LP_STATUS(lpp)) & LP_SELECTED))
	    /* while not ready do busy loop */
	    {
		if (!--wait) {
		    printk("lp%d: timed out\n", target);
		    return 0;
		}

#if NEED_RESCHED
		if (need_resched)
		    schedule();
#endif

	    }
#endif

	if (!(status & LP_ERROR)) { /* printer error */
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

static size_t lp_write(struct inode *inode, struct file *file, char *buf, size_t count)
{
    register char *chrsp;

#ifdef USE_LP_RESET

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

static int lp_open(struct inode *inode, struct file *file)
{
    register struct lp_info *lpp;
    short status;
    unsigned short int target;

    target = MINOR(inode->i_rdev);

#ifdef BIOS_PORTS
    lpp = &ports[target];
#else
    lpp = &ports[port_order[target]];
#endif

    status = lpp->flags;

    if (!(status & LP_EXIST)) { /* if LP_EXIST flag not set */
	debug1("lp: device lp%d doesn't exist\n", target);
	return -ENODEV;
    }

    if (status & LP_BUSY) { /* if LP_BUSY flag set */
	debug1("lp: device lp%d busy\n", target);
	return -EBUSY;
    }

/*
 * nonexistent port can't be busy
 */
    lpp->flags = LP_EXIST | LP_BUSY;

#if 0
    access_count[target]++;
#endif

    return 0;
}

static void lp_release(struct inode *inode, struct file *file)
{
    unsigned short int target = MINOR(inode->i_rdev);

#ifdef BIOS_PORTS
    ports[target].flags = LP_EXIST;	/* not busy */
#else
    ports[port_order[target]].flags = LP_EXIST;	/* not busy */
#endif

#if 0
    access_count[target]--;
#endif

    return;
}

#ifndef BIOS_PORTS

static int lp_probe(register struct lp_info *lp)
{
    register char *waitp;

    /* send 0 to port */
    outb_p((unsigned char) (LP_DUMMY), lp->io);

    /* 5 us delay */
    while (((int)waitp) != LP_CHAR_WAIT)
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
	lp->io = (char *)peekw((word_t) (2 * ((int)ip) + 8), 0x40);
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
