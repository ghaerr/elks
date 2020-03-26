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

#ifdef CONFIG_PSEUDO_TTY

int pty_open(struct inode *inode, struct file *file)
{
    register struct tty *otty;
    register char *pi;

    pi = 0;
    if (!(otty = determine_tty(inode->i_rdev))) {
	debug("failed: NODEV\n");
	pi = (char *)(-ENODEV);
    }
    else if (otty->flags & TTY_OPEN) {
	debug("failed: BUSY\n");
	pi = (char *)(-EBUSY);
    }
    return (int)pi;
}

void pty_release(struct inode *inode, struct file *file)
{
    register struct tty *otty;

    if ((otty = determine_tty(inode->i_rdev)))
	kill_pg(otty->pgrp, SIGHUP, 1);
}

int pty_ioctl(struct inode *inode, struct file *file, int cmd, char *arg)
{
    return -EINVAL;
}

int pty_select (struct inode *inode, struct file *file, int sel_type)
{
	int res = 0;
	register struct tty *tty = determine_tty (inode->i_rdev);

	switch (sel_type) {
		case SEL_IN:
		if (tty->outq.len == 0) {
			select_wait (&tty->outq.wait);
			break;
		}
		res = 1;
		break;

		case SEL_OUT:
		if (tty->inq.len == tty->inq.size) {
			select_wait (&tty->inq.wait);
			break;
		}
		res = 1;
		break;

		default:
		res = -EINVAL;
	}

	return res;
}

size_t pty_read (struct inode *inode, struct file *file, char *data, size_t len)
{
	int count = 0;
	int err;

	struct tty *tty = determine_tty (inode->i_rdev);
	if (tty == NULL) return -EBADF;

	while (count < len) {
		err = chq_wait_rd (&tty->outq, (file->f_flags & O_NONBLOCK) | count);
		if (err < 0) {
			if (count == 0) count = err;
			break;
		}

		put_user_char (tty_outproc (tty), (void *)(data++));
		count++;
		wake_up (&tty->outq.wait);  /* because tty_outproc does not */
	}

	return count;
}

size_t pty_write (struct inode *inode, struct file *file, char *data, size_t len)
{
	int count = 0;
	int err;

	struct tty *tty = determine_tty (inode->i_rdev);
	if (tty == NULL) return -EBADF;

	while (count < len) {
		err = chq_wait_wr (&tty->inq, (file->f_flags & O_NONBLOCK) | count);
		if (err < 0) {
			if (count == 0) count = err;
			break;
		}

		chq_addch (&tty->inq, get_user_char ((void *)(data++)));
		count++;
	}

	return count;
}

int ttyp_write(register struct tty *tty)
{
    if (tty->outq.len == tty->outq.size)
	interruptible_sleep_on(&tty->outq.wait);
    return 0;
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

#endif	/* CONFIG_PSEUDO_TTY */
