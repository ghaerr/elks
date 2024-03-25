/*
 * arch/i86/drivers/char/pty.c - A simple pty driver
 * (C) 1999 Alistair Riddoch
 */

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

/* /dev/ptyp0 master (PTY) open */
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

/* /dev/ptyp0 master close */
void pty_release(struct inode *inode, struct file *file)
{
    register struct tty *otty;

    debug("pty release\n");
    if ((otty = determine_tty(inode->i_rdev)))
	kill_pg(otty->pgrp, SIGHUP, 1);
}

/* /dev/ptyp0 master select */
int pty_select (struct inode *inode, struct file *file, int sel_type)
{
	int res = 0;
	register struct tty *tty = determine_tty (inode->i_rdev);

	switch (sel_type) {
	case SEL_IN:
		debug("pty select(%P) len %d\n", tty->outq.len);
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

/* /dev/ptyp0 master read (from slave /dev/ttyp0 outq to telnetd) */
size_t pty_read (struct inode *inode, struct file *file, char *data, size_t len)
{
	size_t count = 0;
	int err;

	struct tty *tty = determine_tty (inode->i_rdev); /* get slave TTY*/
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

/* /dev/ptyp0 master write (from telnetd to slave /dev/ttyp0 inq) */
size_t pty_write (struct inode *inode, struct file *file, char *data, size_t len)
{
	size_t count = 0;
	int ret;

	struct tty *tty = determine_tty (inode->i_rdev); /* get slave TTY*/
	if (tty == NULL) return -EBADF;

	while (count < len) {
		ret = chq_wait_wr (&tty->inq, (file->f_flags & O_NONBLOCK) | count);
		if (ret < 0) {
			//if (ret == -EINTR) continue;
			if (count == 0) count = ret;
			break;
		}

		ret = get_user_char ((void *)(data++));
		if (!tty_intcheck(tty, ret))
			chq_addch_nowakeup (&tty->inq, ret);
		count++;
	}
	if (count > 0)
		wake_up(&tty->inq.wait);

	return count;
}

/* /dev/ttyp0 slave (TTY) open */
static int ttyp_open(struct tty *tty)
{
	if (tty->usecount++)
		return 0;
	return tty_allocq(tty, PTYINQ_SIZE, PTYOUTQ_SIZE);
}

/* /dev/ttyp0 slave (TTY) close */
static void ttyp_release(struct tty *tty)
{
	debug("TTYP release\n");
	if (--tty->usecount == 0) {
		tty_freeq(tty);
		wake_up(&tty->outq.wait);
	}
}

/* /dev/ttyp0 slave TTY subdriver write - no action, master read will handle transfer */
static int ttyp_write(register struct tty *tty)
{
    return 0;
}

/* /dev/ptyp0 master side is character special file */
static struct file_operations pty_fops = {
    pipe_lseek,			/* Same behavoir, return -ESPIPE */
    pty_read,
    pty_write,
    NULL,
    pty_select,			/* Select - needs doing */
    NULL,			/* ioctl */
    pty_open,
    pty_release
};

/* /dev/ttyp0 slave side is TTY with matching minor number */
struct tty_ops ttyp_ops = {
    ttyp_open,
    ttyp_release,
    ttyp_write,
    NULL,
    NULL,			/* ioctl*/
    NULL                        /* conout */
};

void INITPROC pty_init(void)
{
    register_chrdev(PTY_MASTER_MAJOR, "pty", &pty_fops);
}

#endif	/* CONFIG_PSEUDO_TTY */
