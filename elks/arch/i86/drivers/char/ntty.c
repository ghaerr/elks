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
#if defined(CONFIG_CONSOLE_DIRECT) || defined(CONFIG_SIBO_CONSOLE_DIRECT)
extern struct tty_ops dircon_ops;
#endif
#ifdef CONFIG_CONSOLE_BIOS
extern struct tty_ops bioscon_ops;
#endif
#ifdef CONFIG_CHAR_DEV_RS
extern struct tty_ops rs_ops;
#endif
#ifdef CONFIG_PSEUDO_TTY
extern struct tty_ops ttyp_ops;
#endif

int tty_intcheck(register struct tty *ttyp, unsigned char key)
{
    register char *psig = 0;

    if ((ttyp->termios.c_lflag & ISIG) && (ttyp->pgrp)) {
	if (key == ttyp->termios.c_cc[VINTR])
	    psig = (char *) SIGINT;
	if (key == ttyp->termios.c_cc[VSUSP])
	    psig = (char *) SIGTSTP;
	if (psig) {
	    kill_pg(ttyp->pgrp, (sig_t) psig, 1);
	    return 1;
	}
    }
    return 0;
}

/* Turns a dev_t variable into its tty, or NULL if it's not valid */

struct tty *determine_tty(dev_t dev)
{
    register struct tty *ttyp = &ttys[0];

    do {
	if (ttyp->minor == (unsigned short int)MINOR(dev))
	    return ttyp;
    } while (++ttyp < &ttys[MAX_TTYS]);

    return 0;
}

int tty_open(struct inode *inode, struct file *file)
{
    register struct tty *otty;
    register __ptask currentp = current;
    int err;

    if (!(otty = determine_tty(inode->i_rdev)))
	return -ENODEV;

#if 0
    memcpy(&otty->termios, &def_vals, sizeof(struct termios));
#endif

    err = otty->ops->open(otty);
    if (err)
	return err;
    if (otty->pgrp == 0 && currentp->session == currentp->pid
	&& currentp->tty == NULL) {
	otty->pgrp = currentp->pgrp;
	currentp->tty = otty;
    }
    otty->flags |= TTY_OPEN;
    chq_init(&otty->inq, otty->inq_buf, INQ_SIZE);
    chq_init(&otty->outq, otty->outq_buf, OUTQ_SIZE);
    return 0;
}

int ttynull_openrelease(struct tty *tty)
{
    return 0;
}

void tty_release(struct inode *inode, struct file *file)
{
    register struct tty *rtty;

    rtty = determine_tty(inode->i_rdev);
    if (!rtty)
	return;

    if (current->pid == rtty->pgrp) {
	kill_pg(rtty->pgrp, SIGHUP, 1);
	rtty->pgrp = 0;
    }
    rtty->flags &= ~TTY_OPEN;
    rtty->ops->release(rtty);
}

static void tty_charout_raw(register struct tty *tty, unsigned char ch)
{
    chq_addch(&tty->outq, ch, 1);	/* Will block */
    tty->ops->write(tty);
}

/* Write 1 byte to a terminal, with processing */
void tty_charout(register struct tty *tty, unsigned char ch)
{
    switch (ch) {
    case '\t':
	if (tty->termios.c_lflag & ICANON) {
	    register char *pi = (char *) TAB_SPACES;
	    do {
		tty_charout_raw(tty, ' ');
	    } while (--pi);
	    return;
	}
	break;
    case '\n':
	if (tty->termios.c_oflag & ONLCR)
	    tty_charout_raw(tty, '\r');
    }
    tty_charout_raw(tty, ch);
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

size_t tty_write(struct inode *inode, struct file *file, char *data, int len)
{
    register struct tty *tty = determine_tty(inode->i_rdev);
    register char *pi;
#if 0
    int blocking = (file->f_flags & O_NONBLOCK) ? 0 : 1;
#endif

    pi = (char *)len;
    while ((int)(pi--)) {
	tty_charout(tty,
	    get_user_char((void *)(data++))
	   /* , blocking */ );
    }
    return len;
}

size_t tty_read(struct inode *inode, struct file *file, char *data, int len)
{
#if 1
    register struct tty *tty = determine_tty(inode->i_rdev);
    register char *pi = 0;
    int j, k;
    int rawmode = (tty->termios.c_lflag & ICANON) ? 0 : 1;
    int blocking = (file->f_flags & O_NONBLOCK) ? 0 : 1;
    unsigned char ch;

    if (len != 0) {
	do {
	    if (tty->ops->read) {
		tty->ops->read(tty);
		blocking = 0;
	    }
	    j = chq_getch(&tty->inq, &ch, blocking);

	    if (j == -1) {
		if (!blocking)
		    break;
		return -EINTR;
	    }
	    if (!rawmode && (j == 04))	/* CTRL-D */
		break;
	    if (rawmode || (j != '\b')) {
		put_user_char(ch, (void *)(data++));
		++pi;
		tty_echo(tty, ch);
	    } else if (((int)pi) > 0) {
		--pi;
		k = ((get_user_char((void *)(--data))
		      == '\t') ? TAB_SPACES : 1);
		do {
		    tty_echo(tty, ch);
		} while (--k);
	    }
	} while (((int)pi) < len && (rawmode || j != '\n'));
    }
    return (int) pi;


#else
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
	    put_user_char(ch, (void *)(data + i++));
	    tty_echo(tty, ch);
	} else if (i > 0) {
	    lch = ((get_user_char((void *)(data + --i)) == '\t')
			? TAB_SPACES : 1);
	    for (k = 0; k < lch; k++)
		tty_echo(tty, ch);
	}
    } while (i < len && (rawmode || j != '\n'));

    return i;
#endif
}

int tty_ioctl(struct inode *inode, struct file *file, int cmd, char *arg)
{
    register struct tty *tty = determine_tty(inode->i_rdev);
    register char *ret;

    switch (cmd) {
    case TCGETS:
	ret = (char *)verified_memcpy_tofs(arg, &tty->termios, sizeof(struct termios));
	break;
    case TCSETS:
    case TCSETSW:
    case TCSETSF:
	ret = (char *)verified_memcpy_fromfs(&tty->termios, arg, sizeof(struct termios));

	/* Inform driver that things have changed */
	if (tty->ops->ioctl != NULL)
	    tty->ops->ioctl(tty, cmd, arg);
	break;
    default:
	ret = (char *)((tty->ops->ioctl == NULL)
	    ? -EINVAL
	    : tty->ops->ioctl(tty, cmd, arg));
    }
    return (int)ret;
}

int tty_select(struct inode *inode,	/* how revolting, K&R style defs */
	       struct file *file, int sel_type)
{
    register struct tty *tty = determine_tty(inode->i_rdev);
    register char *ret = 0;

    switch (sel_type) {
    case SEL_IN:
	if (chq_peekch(&tty->inq))
	    return 1;
	select_wait(&tty->inq.wq);
	break;
    case SEL_OUT:
	ret = (char *)(!chq_full(&tty->outq));
	if (!ret)
	    select_wait(&tty->outq.wq);
	    /*@fallthrough@*/
/*      case SEL_EX: */
/*  	break; */
    }
    return (int) ret;
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
/*      unsigned short int i; */
    register char *pi;

    ttyp = ttys;
    while(ttyp < &ttys[NUM_TTYS]) {
	ttyp->minor = (unsigned short int)(-1L); /* set unsigned to -1 */
	memcpy(&ttyp->termios, &def_vals, sizeof(struct termios));
	ttyp++;
    }

#if defined(CONFIG_CONSOLE_DIRECT) || defined(CONFIG_SIBO_CONSOLE_DIRECT) || defined(CONFIG_CONSOLE_BIOS)

    ttyp = ttys;
    for (pi = 0 ; ((int)pi) < 4 ; pi++) {
#ifdef CONFIG_CONSOLE_BIOS
	ttyp->ops = &bioscon_ops;
#else
	ttyp->ops = &dircon_ops;
#endif
	chq_init(&ttyp->inq, ttyp->inq_buf, INQ_SIZE);
	(ttyp++)->minor = (int)pi;
    }

#endif

#ifdef CONFIG_CHAR_DEV_RS

    ttyp = &ttys[4];
    for (pi = (char *)4; ((int)pi) < 4+NR_SERIAL; pi++) {
	ttyp->ops = &rs_ops;
	(ttyp++)->minor = ((int)pi) + 60;
    }

#endif

#ifdef CONFIG_PSEUDO_TTY

    ttyp = &ttys[4+NR_SERIAL];
    for (pi = 4+(char *)NR_SERIAL; ((int)pi) < 4+NR_SERIAL+NR_PTYS; pi++) {
	ttyp->ops = &ttyp_ops;
	(ttyp++)->minor = (int)pi;
    }
    pty_init();

#endif

    register_chrdev(TTY_MAJOR, "tty", &tty_fops);
}
