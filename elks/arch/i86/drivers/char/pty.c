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

int pty_open(struct inode *inode, struct file *file)
{
    struct tty *otty;

    if (otty = determine_tty(inode->i_rdev)) {
	printk("pty_open() %x", otty);
	if (otty->flags & TTY_OPEN)
	    return -EBUSY;
	printk(" succeeded\n");
	return 0;
    } else {
	printk(" failed\n");
	return -ENODEV;
    }
}

int pty_release(struct inode *inode, struct file *file)
{
    struct tty *otty;

    if (otty = determine_tty(inode->i_rdev))
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
}

int pty_read(struct inode *inode, struct file *file, char *data, int len)
{
    register struct tty *tty = determine_tty(inode->i_rdev);
    int i = 0, j, l;
    unsigned char ch;

    printk("pty_read()");
    if (tty == NULL)
	return -ENODEV;
    l = (file->f_flags & O_NONBLOCK) ? 0 : 1;
    while (i < len) {
	j = chq_getch(&tty->outq, &ch, l);
	if (j == -1)
	    if (l)
		return -EINTR;
	    else
		break;
	printk(" rc[%u,%u]", i, len);
	pokeb(current->t_regs.ds, (data + i++), ch);
    }
    printk("{%u}\n", i);
    return i;
}

int pty_write(struct inode *inode, struct file *file, char *data, int len)
{
    register struct tty *tty = determine_tty(inode->i_rdev);
    int i = 0, l;
    char ch;

    printk("pty_write()");
    if (tty == NULL)
	return -ENODEV;
    l = (file->f_flags & O_NONBLOCK) ? 0 : 1;
    while (i < len) {
	ch = peekb(current->t_regs.ds, data + i);
	if (chq_addch(&tty->inq, ch, l) == -1)
	    if (l)
		return -EINTR;
	    else
		break;
	i++;
	printk(" wc");
    }
    printk("\n");
    return i;
}

static struct file_operations pty_fops = {
    pipe_lseek,			/* Same behavoir, return -ESPIPE */
    pty_read,
    pty_write,
    NULL,
    pty_select,			/* Select - needs doing */
    pty_ioctl,			/* ioctl */
    pty_open,
    pty_release,
#ifdef BLOAT_FS
    NULL,
    NULL,
    NULL
#endif
};

int ttyp_write(struct tty *tty)
{
    if (tty->outq.len == tty->outq.size)
	interruptible_sleep_on(&tty->outq.wq);
}

struct tty_ops ttyp_ops = {
    ttynull_openrelease,	/* None of these really need to do anything */
    ttynull_openrelease,
    ttyp_write,
    NULL,
    NULL,
};

void pty_init(void)
{
    register_chrdev(PTY_MASTER_MAJOR, "pty", &pty_fops);
}
