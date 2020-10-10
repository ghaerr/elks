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

    if (!(otty = determine_tty(inode->i_rdev))) {
	debug("pty open fail NODEV\n");
	return -ENODEV;
    }
    if (otty->flags & TTY_OPEN) {
	debug("pty open fail BUSY\n");
	return -EBUSY;
    }
    return 0;
}

void pty_release(struct inode *inode, struct file *file)
{
    register struct tty *otty;

    debug("pty release\n");
    if ((otty = determine_tty(inode->i_rdev)))
	kill_pg(otty->pgrp, SIGHUP, 1);
}

int pty_select (struct inode *inode, struct file *file, int sel_type)
{
	int res = 0;
	register struct tty *tty = determine_tty (inode->i_rdev);

	switch (sel_type) {
		case SEL_IN:
		debug("pty select(%d)\n", current->pid);
		if (tty->outq.len == 0 && tty->usecount) {
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

	/* return EOF on master closed*/
	if (!tty->usecount)
		return 0;

	while (count < len) {
		err = chq_wait_rd (&tty->outq, (file->f_flags & O_NONBLOCK) | count);
		if (err < 0) {
			if (count == 0) count = err;
			break;
		}

		put_user_char (tty_outproc (tty), (void *)(data++));
		count++;
	}

	if (count > 0)
		 wake_up(&tty->outq.wait);  /* because ttyoutproc does not*/
	return count;
}

size_t pty_write (struct inode *inode, struct file *file, char *data, size_t len)
{
	int count = 0;
	register int ret;

	struct tty *tty = determine_tty (inode->i_rdev);
	if (tty == NULL) return -EBADF;

	while (count < len) {
		ret = chq_wait_wr (&tty->inq, (file->f_flags & O_NONBLOCK) | count);
		if (ret < 0) {
			if (count == 0) count = ret;
			break;
		}

		ret = get_user_char ((void *)(data++));
		if (!tty_intcheck(tty, ret)) {
			chq_addch_nowakeup (&tty->inq, ret);
			count++;
		}
	}
	if (count > 0)
		wake_up(&tty->inq.wait);

	return count;
}

static int ttyp_open(struct tty *tty)
{
	if (tty->usecount++)
		return 0;
	return tty_allocq(tty, PTYINQ_SIZE, PTYOUTQ_SIZE);
}

static void ttyp_release(struct tty *tty)
{
	debug("TTYP release\n");
	if (--tty->usecount == 0) {
		tty_freeq(tty);
		wake_up(&tty->outq.wait);
	}
}

static int ttyp_write(register struct tty *tty)
{
    return 0;
}

/*@-type@*/

static struct file_operations pty_fops = {
    pipe_lseek,			/* Same behavoir, return -ESPIPE */
    pty_read,
    pty_write,
    NULL,
    pty_select,			/* Select - needs doing */
    NULL,			/* ioctl */
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
    ttyp_open,
    ttyp_release,
    ttyp_write,
    NULL,
    NULL			/* ioctl*/
};

/*@+type@*/

void pty_init(void)
{
    register_chrdev(PTY_MASTER_MAJOR, "pty", &pty_fops);
}

#endif	/* CONFIG_PSEUDO_TTY */
