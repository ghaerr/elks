/*
 * arch/i86/drivers/char/tty.c - A simple tty driver
 * (C) 1997 Chad Page et. al
 *
 * Modified by Greg Haerr <greg@censoft.com> for screen editors
 * 
 * 03/27/2001 : Modified for raw mode support (HarKal)
 */

/* 
 * This new tty sub-system builds around the character queue to provide a
 * VFS interface to the character drivers (what a mouthful! :)  
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
#include <linuxmt/init.h>
#include <linuxmt/major.h>

/*
 * XXX plac: setting default to B9600 instead of B1200
 */
struct termios def_vals = { BRKINT,
    ONLCR,
    (tcflag_t) (B9600 | CS8),
    (tcflag_t) (ECHO | ICANON | ISIG),
    0,
    {3, 28, 127, 21, 4, 0, 1, 0, 17, 19, 26, 0, 18, 15, 23, 22, 0, 0, 0}
};

#define TAB_SPACES 8

#define MAX_TTYS NUM_TTYS
struct tty ttys[MAX_TTYS];
extern struct tty_ops dircon_ops;
extern struct tty_ops bioscon_ops;
extern struct tty_ops rs_ops;
extern struct tty_ops ttyp_ops;

int tty_intcheck(register struct tty *ttyp, unsigned char key)
{
    sig_t sig = 0;

    if ((ttyp->termios.c_lflag & ISIG) && (ttyp->pgrp)) {
	if (key == ttyp->termios.c_cc[VINTR])
	    sig = (sig_t) SIGINT;
	if (key == ttyp->termios.c_cc[VSUSP])
	    sig = (sig_t) SIGTSTP;
	if (sig) {
	    kill_pg(ttyp->pgrp, sig, 1);
	    return 1;
	}
    }
    return 0;
}

/* Turns a dev_t variable into its tty, or NULL if it's not valid */

struct tty *determine_tty(dev_t dev)
{
    register struct tty *ttyp;
    unsigned short int i, minor = MINOR(dev);

    for (i = 0; i < MAX_TTYS; i++) {
	ttyp = &ttys[i];
	if (ttyp->minor == minor)
	    return ttyp;
    }
    return 0;
}

int tty_open(register struct inode *inode, struct file *file)
{
    register struct tty *otty;
    int err;

    if ((otty = determine_tty(inode->i_rdev))) {

#if 0
	memcpy(&otty->termios, &def_vals, sizeof(struct termios));
#endif

	err = otty->ops->open(otty);
	if (err)
	    return err;
	if (otty->pgrp == NULL && current->session == current->pid
	    && current->tty == NULL) {
	    otty->pgrp = current->pgrp;
	    current->tty = otty;
	}
	otty->flags |= TTY_OPEN;
	chq_init(&otty->inq, otty->inq_buf, INQ_SIZE);
	chq_init(&otty->outq, otty->outq_buf, OUTQ_SIZE);
	return 0;
    } else
	return -ENODEV;
}

int ttynull_openrelease(struct tty *tty)
{
    return 0;
}

int tty_release(register struct inode *inode, struct file *file)
{
    register struct tty *rtty;
    rtty = determine_tty(inode->i_rdev);
    if (rtty) {
	if (current->pid == rtty->pgrp) {
	    kill_pg(rtty->pgrp, SIGHUP, 1);
	    rtty->pgrp = NULL;
	}
	rtty->flags &= ~TTY_OPEN;
	return rtty->ops->release(rtty);
    } else
	return -ENODEV;
}

static void tty_charout_raw(register struct tty *tty, unsigned char ch)
{
    chq_addch(&tty->outq, ch, 1);	/* Will block */
    tty->ops->write(tty);
}

/* Write 1 byte to a terminal, with processing */
void tty_charout(register struct tty *tty, unsigned char ch)
{
    int j;

    switch (ch) {
    case '\t':
	if (!(tty->termios.c_lflag & ICANON))
	    tty_charout_raw(tty, '\t');
	else
	    for (j = 0; j < TAB_SPACES; j++)
		tty_charout_raw(tty, ' ');
	break;
    case '\n':
	if (tty->termios.c_oflag & ONLCR)
	    tty_charout(tty, '\r');
    default:
	tty_charout_raw(tty, ch);
	break;
    }
}

void tty_echo(register struct tty *tty, unsigned char ch)
{
    if (tty->termios.c_lflag & ECHO)
	tty_charout(tty, ch);
}

/*
 *	Write to a terminal
 *
 */

int tty_write(struct inode *inode, struct file *file, char *data, int len)
{
    register struct tty *tty = determine_tty(inode->i_rdev);
    int i = 0;
#if 0
    int blocking = (file->f_flags & O_NONBLOCK) ? 0 : 1;
#endif
    __u16 tmp;

    while (i < len) {
	tmp = peekb(current->t_regs.ds, (__u16) (data + i++));
	tty_charout(tty, (unsigned char) tmp /* , blocking */ );
    }
    tty->ops->write(tty);
    return i;
}

int tty_read(struct inode *inode, struct file *file, char *data, int len)
{
    register struct tty *tty = determine_tty(inode->i_rdev);
    int i = 0, j = 0, k, lch;
    int rawmode = (tty->termios.c_lflag & ICANON) ? 0 : 1;
    int blocking = (file->f_flags & O_NONBLOCK) ? 0 : 1;
    unsigned char ch;

    if (len == 0)
	return 0;

    do {
	if (tty->ops->read) {
	    tty->ops->read(tty);
	    blocking = 0;
	}
	j = chq_getch(&tty->inq, &ch, blocking);

	if (j == -1)
	    if (blocking)
		return -EINTR;
	    else
		break;
	if (!rawmode && (j == 04))	/* CTRL-D */
	    break;
	if (rawmode || (j != '\b')) {
	    pokeb(current->t_regs.ds, (__u16) (data + i++), ch);
	    tty_echo(tty, ch);
	} else if (i > 0) {
	    lch = ((peekb(current->t_regs.ds, (__u16) (data + --i)) == '\t')
			? TAB_SPACES : 1);
	    for (k = 0; k < lch; k++)
		tty_echo(tty, ch);
	}
    } while (i < len && (rawmode || j != '\n'));

    return i;
}

int tty_ioctl(struct inode *inode, struct file *file, int cmd, char *arg)
{
    register struct tty *tty = determine_tty(inode->i_rdev);
    int ret;

    switch (cmd) {
    case TCGETS:
	ret = verified_memcpy_tofs(arg, &tty->termios, sizeof(struct termios));
	break;
    case TCSETS:
    case TCSETSW:
    case TCSETSF:
	ret =
	    verified_memcpy_fromfs(&tty->termios, arg, sizeof(struct termios));

	/* Inform driver that things have changed */
	if (tty->ops->ioctl != NULL)
	    tty->ops->ioctl(tty, cmd, arg);
	break;
    default:
	if (tty->ops->ioctl == NULL)
	    ret = -EINVAL;
	else
	    ret = tty->ops->ioctl(tty, cmd, arg);
	break;
    }
    return ret;
}

int tty_select(struct inode *inode,	/* how revolting, K&R style defs */
	       struct file *file, int sel_type)
{
    register struct tty *tty = determine_tty(inode->i_rdev);
    int ret = 0;

    switch (sel_type) {
    case SEL_IN:
	if (chq_peekch(&tty->inq))
	    return 1;
	select_wait(&tty->inq.wq);
	break;
    case SEL_OUT:
	ret = !chq_full(&tty->outq);
	if (!ret)
	    select_wait(&tty->outq.wq);
	    /*@fallthrough@*/
    case SEL_EX:
	break;
    }
    return ret;
}

/*@-type@*/

static struct file_operations tty_fops = {
    pipe_lseek,			/* Same behavoir, return -ESPIPE */
    tty_read,
    tty_write,
    NULL,
    tty_select,			/* Select - needs doing */
    tty_ioctl,			/* ioctl */
    tty_open,
    tty_release
#ifdef BLOAT_FS
	,
    NULL,
    NULL,
    NULL
#endif
};

/*@+type@*/

void tty_init(void)
{
    register struct tty *ttyp;
    unsigned short int i;

    for (i = 0; i < NUM_TTYS; i++) {
	ttyp = &ttys[i];
	ttyp->minor -= (ttyp->minor + 1);	/* set unsigned to -1 */
	memcpy(&ttyp->termios, &def_vals, sizeof(struct termios));
    }

#ifdef CONFIG_CONSOLE_BIOS

    ttyp = &ttys[0];
    ttyp->ops = &bioscon_ops;
    ttyp->minor = 0;

#endif

#if defined(CONFIG_CONSOLE_DIRECT) || defined(CONFIG_SIBO_CONSOLE_DIRECT)

    for (i = 0; i < 3; i++) {
	ttyp = &ttys[i];
	if (i == 0) {
	    chq_init(&ttyp->inq, ttyp->inq_buf, INQ_SIZE);
	}
	ttyp->ops = &dircon_ops;
	ttyp->minor = i;
    }

#endif

#ifdef CONFIG_CHAR_DEV_RS

    for (i = 4; i < 8; i++) {
	ttyp = &ttys[i];
	ttyp->ops = &rs_ops;
	ttyp->minor = i + 60;
    }

#endif

#ifdef CONFIG_PSEUDO_TTY

    for (i = 8; i < 8 + NR_PTYS; i++) {
	ttyp = &ttys[i];
	ttyp->ops = &ttyp_ops;
	ttyp->minor = i;
    }
    pty_init();

#endif

    register_chrdev(TTY_MAJOR, "tty", &tty_fops);
}
