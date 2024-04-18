/*
 * lp driver for ELKS kernel
 * Copyright (C) 1997 Blaz Antonic
 * Debugged by Patrick Lam and Chuck Coffing
 */

#include <linuxmt/config.h>

#ifdef CONFIG_CHAR_DEV_LP

#include <linuxmt/errno.h>
#include <linuxmt/kernel.h>
#include <linuxmt/lp.h>
#include <linuxmt/mm.h>
#include <linuxmt/major.h>
#include <linuxmt/sched.h>
#include <linuxmt/types.h>
#include <linuxmt/debug.h>

#include <arch/io.h>

struct lp_info {
    unsigned short io;
    char flags;
};

#ifdef BIOS_PORTS

/* We'll get port info from BIOS. There are 4 ports max. */
static struct lp_info ports[LP_PORTS];

#else

/* Probe the standard ports. */
static struct lp_info ports[LP_PORTS] = {
    {0x3BC, 0},
    {0x378, 0},
    {0x278, 0},
};

#endif

static void
delay(int usec)
{
    while (usec > 0) {
        outb(0, 0x80);
        --usec;
    }
}

static void
lp_reset(struct lp_info *lpp)
{
    LP_CONTROL(0, lpp);
    delay(LP_RESET_WAIT);
    LP_CONTROL(LP_SELECT | LP_INIT, lpp);
}

static int
lp_char_polled(unsigned char c, struct lp_info *lpp)
{
    int count = LP_CHAR_WAIT;

    /* write byte to data port first */
    outb(c, lpp->io);

    /* wait for !busy */
    do {
        if (LP_STATUS(lpp) & LP_PBUSY) {
            /* strobe to latch char */
            LP_CONTROL(LP_SELECT | LP_INIT | LP_STROBE, lpp);
            LP_CONTROL(LP_SELECT | LP_INIT, lpp);
            debug_lp("lp: out %x to %x\n", c, lpp->io);
            /* most software impls skip checking ack */
            return 1;
        }
    } while (count-- > 0);

    return 0;
}

static size_t
lp_write(struct inode *inode, struct file *file, char *buf, size_t count)
{
    jiff_t timeout = jiffies + LP_TIME_WAIT;
    unsigned short target;
    struct lp_info *lpp;
    size_t retval;
    size_t chrsp = 0;
    int status;

    target = MINOR(inode->i_rdev);

    lpp = &ports[target];

    while (chrsp < count) {
        debug_lp("lp%d: write %d/%d\n", target, chrsp, count);
        if (lp_char_polled(get_user_char((void *)(buf++)), lpp)) {
            chrsp++;
            continue;
        } else {
            /* printer timed out */
            status = LP_STATUS(lpp);

            if (status & LP_POUTPA) {
                printk("lp%d out of paper\n", target);
                retval = -ENOSPC;
            } else if (!(status & LP_PSELECD)) {
                printk("lp%d off-line\n", target);
                retval = -EIO;
            } else if (!(status & LP_PERRORP)) {
                printk("lp%d printer error\n", target);
                retval = -EFAULT;
            } else if (timeout >= jiffies) {
                debug_lp("lp%d: timeout %d<%d\n", target, chrsp, count);
                retval = 0;
            } else {
                continue;
            }

            return chrsp ? chrsp : retval;
        }

    }
    return chrsp;
}

static int
lp_open(struct inode *inode, struct file *file)
{
    struct lp_info *lpp;
    short status;
    unsigned short target;

    target = MINOR(inode->i_rdev);

    lpp = &ports[target];

    status = lpp->flags;

    if (!(status & LP_EXIST)) { /* if LP_EXIST flag not set */
        debug_lp("lp: device lp%d doesn't exist at %x\n", target, lpp->io);
        return -ENODEV;
    }

    if (status & LP_BUSY) {     /* if LP_BUSY flag set */
        debug_lp("lp: device lp%d busy\n", target);
        return -EBUSY;
    }

/*
 * nonexistent port can't be busy
 */
    lpp->flags = LP_EXIST | LP_BUSY;

    return 0;
}

static void
lp_release(struct inode *inode, struct file *file)
{
    unsigned short target = MINOR(inode->i_rdev);

    ports[target].flags = LP_EXIST;     /* not busy */

    return;
}

#ifndef BIOS_PORTS

static int
lp_probe(struct lp_info *lp)
{
    /* send 0 to port */
    outb_p((unsigned char)(LP_DUMMY), lp->io);

    /* 0 expected; 255 returned if nonexistent port */
    if (inb_p(lp->io) == LP_DUMMY) {
        lp->flags = LP_EXIST;
        lp_reset(lp);
        return 0;
    }
    return -1;
}

#endif

static struct file_operations lp_fops = {
    NULL,                       /* lseek */
    NULL,                       /* read */
    lp_write,                   /* write */
    NULL,                       /* readdir */
    NULL,                       /* select */
    NULL,                       /* ioctl */
    lp_open,                    /* open */
    lp_release                  /* release */
};

void INITPROC
lp_init(void)
{
    struct lp_info *lp = &ports[0];
    int i;
    int count = 0;

#ifdef BIOS_PORTS

    /* only ports 0, 1, 2 and 3 may exist according to RB's intlist */
    for (i = 0; i < LP_PORTS; i++) {
        /* 8 is offset for LPT info, 2 bytes for each entry */
        lp->io = peekw(2 * i + 8, 0x40);
        /* returns 0 if port wasn't detected by BIOS at bootup */
        if (!lp->io)
            break;              /* there can be no more ports */
        printk("lp%d at 0x%x, using polling driver\n", i, lp->io);
        lp->flags = LP_EXIST;
        lp_reset(lp);
        lp++;
    }
    count = i;

#else

    /* probe for ports */
    for (i = 0; i < LP_PORTS; i++) {
        if (!lp_probe(lp)) {
            printk("lp%d at 0x%x, using polling driver\n", count, lp->io);
            if (count != i)
                ports[count] = *lp;
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

#endif                          /* CONFIG_CHAR_DEV_LP */
