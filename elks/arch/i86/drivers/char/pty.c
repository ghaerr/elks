/*
 * arch/i86/drivers/char/pty.c - A simple pty driver
 * (C) 1999 Alistair Riddoch
 */

#include <linuxmt/types.h>
#include <linuxmt/config.h>
#include <linuxmt/sched.h>
#include <linuxmt/fs.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/errno.h>
#include <linuxmt/mm.h>
#include <linuxmt/termios.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/ntty.h>
#include <linuxmt/major.h>
#include <linuxmt/debug.h>

int pty_open(struct inode *inode, struct file *file)
{
    register struct tty *otty;

    debug1("PTY: open() %x ", otty);
    if ((otty = determine_tty(inode->i_rdev))) {
	if (otty->flags & TTY_OPEN) {
	    debug("failed: BUSY\n");
	    return -EBUSY;
	}
	debug("succeeded\n");
	return 0;
    }
    debug("failed: NODEV\n");
    return -ENODEV;
}

int pty_release(struct inode *inode, struct file *file)
{
    register struct tty *otty;

    if ((otty = determine_tty(inode->i_rdev)))
	kill_pg(otty->pgrp, SIGHUP, 1);
    return 0;
}

int pty_ioctl(struct inode *inode, struct file *file, int cmd, char *arg)
{
    return -EINVAL;
}

int pty_select(struct inode *inode, struct file *file, int sel_type)
{
    register struct tty *tty = determine_tty(inode->i_rdev);
#if 1

    /* Since everything else was disabled... */
    if (sel_type == SEL_IN) {
	if (chq_peekch(&tty->outq))
	    return 1;
	select_wait(&tty->outq.wq);
    }
    return 0;

#else
    int ret = 0;

    switch (sel_type) {
    case SEL_IN:
	if (chq_peekch(&tty->outq))
	    ret = 1;
	else
	    select_wait(&tty->outq.wq);
	break;
    case SEL_OUT:

#if 0

	if (!chq_full(&tty->inq))
	    return 1;
	select_wait(&tty->inq.wq);
    case SEL_EX:		/* fall thru! */

#endif

	break;
    }
    return ret;
#endif
}

int pty_read(struct inode *inode, struct file *file, char *data, int len)
{
    register struct tty *tty = determine_tty(inode->i_rdev);
    register char *pi;
    int j, l;
    unsigned char ch;

    debug("PTY: read ");
    if (tty == NULL) {
	debug("failed: NODEV\n");
	return -ENODEV;
    }
    l = (file->f_flags & O_NONBLOCK) ? 0 : 1;
    pi = 0;
    while (((int)pi) < len) {
	j = chq_getch(&tty->outq, &ch, l);
	if (j == -1)
	    if (l) {
		debug("failed: INTR\n");
		return -EINTR;
	    } else
		break;
	debug2(" rc[%u,%u]", (int)pi, len);
	pokeb(current->t_regs.ds, (__u16) (data + ((int)pi)), ch);
	++pi;
    }
    debug1("{%u}\n", (int)pi);
    return (int)pi;
}

int pty_write(struct inode *inode, struct file *file, char *data, int len)
{
    register struct tty *tty = determine_tty(inode->i_rdev);
    register char *pi;
    int l;
    unsigned char ch;

    debug("PTY: write ");
    if (tty == NULL) {
	debug("failed: NODEV\n");
	return -ENODEV;
    }
    l = (file->f_flags & O_NONBLOCK) ? 0 : 1;
    pi = 0;
    while (((int)pi) < len) {
	ch = (unsigned char) peekb(current->t_regs.ds, (__u16) (data + ((int)pi)));
	if (chq_addch(&tty->inq, ch, l) == -1)
	    if (l) {
		debug("failed: INTR\n");
		return -EINTR;
	    } else
		break;
	pi++;
	debug(" wc");
    }
    debug("\n");
    return (int)pi;
}

int ttyp_write(register struct tty *tty)
{
    if (tty->outq.len == tty->outq.size)
	interruptible_sleep_on(&tty->outq.wq);
}

/*@-type@*/

static struct file_operations pty_fops = {
    pipe_lseek,			/* Same behavoir, return -ESPIPE */
    pty_read,
    pty_write,
    NULL,
    pty_select,			/* Select - needs doing */
    pty_ioctl,			/* ioctl */
    pty_open,
    pty_release
#ifdef BLOAT_FS
	,
    NULL,
    NULL,
    NULL
#endif
};

struct tty_ops ttyp_ops = {
    ttynull_openrelease,	/* None of these really need to do anything */
    ttynull_openrelease,
    ttyp_write,
    NULL,
    NULL,
};

/*@+type@*/

void pty_init(void)
{
    register_chrdev(PTY_MASTER_MAJOR, "pty", &pty_fops);
}
