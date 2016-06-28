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
struct termios def_vals = { BRKINT|ICRNL,
    OPOST|ONLCR,
    (tcflag_t) (B9600 | CS8),
    (tcflag_t) (ISIG | ICANON | ECHO | ECHOE | ECHONL),
    0,
    {3, 28, /*127*/010, 21, 4, 0, 1, 0, 17, 19, 26, 0, 18, 15, 23, 22, 0, 0, 0}
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
	if (psig)
	    kill_pg(ttyp->pgrp, (sig_t) psig, 1);
    }
    return (int)psig;
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

/*
 * Gets 1 byte from tty output buffer, with processing
 * Used by the function reading bytes from the output
 * buffer to send them to the tty device, using the
 * construct:
 *
 * while(tty->outq.len > 0) {
 * 	ch = tty_outproc(tty);
 * 	write_char_to_device(device, (char)ch);
 * 	cnt++;
 * }
 *
 */
int tty_outproc(register struct tty *tty)
{
    register char *t_oflag;	/* WARNING: highly arch dependent! */
    int ch;

    ch = tty->outq.buf[tty->outq.tail];
    if((int)(t_oflag = (char *)tty->termios.c_oflag) & OPOST) {
	switch(tty->ostate & ~0x0F) {
	case 0x20:		/* Expand tabs to spaces */
	    ch = ' ';
	    if(--tty->ostate & 0xF)
		break;
	case 0x10:		/* Expand NL -> CR NL */
	    tty->ostate = 0;
	    break;
	default:
	    switch(ch) {
	    case '\n':
		if((unsigned)t_oflag & ONLCR) {
		    ch = '\r';				/* Expand NL -> CR NL */
		    tty->ostate = 0x10;
		}
		break;
#if 0
	    case '\r':
		if((unsigned)t_oflag & OCRNL)
		    ch = '\n';				/* Map CR to NL */
		break;
#endif
	    case '\t':
		if(((unsigned)t_oflag & TABDLY) == TAB3) {
		    ch = ' ';				/* Expand tabs to spaces */
		    tty->ostate = 0x20 + TAB_SPACES - 1;
		}
	    }
	}
    }
    if(!tty->ostate) {
	tty->outq.tail++;
	tty->outq.tail &= (tty->outq.size - 1);
	tty->outq.len--;
    }
    return ch;
}

void tty_echo(register struct tty *tty, unsigned char ch)
{
    if((tty->termios.c_lflag & ECHO)
		/*|| ((tty->termios.c_lflag & ECHONL) && (ch == '\n'))*/) {
	chq_addch(&tty->outq, ch, 0);
/*	if((ch == '\b') && (tty->termios.c_lflag & ECHOE)) {
	    chq_addch(&tty->outq, ' ', 0);
	    chq_addch(&tty->outq, '\b', 0);
	}*/
	tty->ops->write(tty);
    }
}

/*
 *	Write to a terminal
 *
 */

size_t tty_write(struct inode *inode, struct file *file, char *data, int len)
{
    register struct tty *tty = determine_tty(inode->i_rdev);
    register char *pi;
    int s;

    pi = 0;
    while(((int)pi) < len) {
	s = chq_addch(&tty->outq,
		      get_user_char((void *)(data++)),
		      !(file->f_flags & O_NONBLOCK));
	tty->ops->write(tty);
	if(s < 0) {
	    if((int)pi == 0)
		pi = (char *)s;
	    break;
	}
	pi++;
    }
    return (size_t)pi;
}

size_t tty_read(struct inode *inode, struct file *file, char *data, int len)
{
    register struct tty *tty = determine_tty(inode->i_rdev);
    register char *pi = 0;
    int ch, k, icanon;
    int blocking = !(file->f_flags & O_NONBLOCK);

    if(len > 0) {
	icanon = tty->termios.c_lflag & ICANON;
	do {
	    if (tty->ops->read) {
		tty->ops->read(tty);
		blocking = 0;
	    }
	    ch = chq_getch(&tty->inq, blocking);

	    if(ch < 0) {
		if((int)pi == 0)
		    pi = (char *)ch;
		break;
	    }
	    if(icanon) {
		if(ch == /*tty->termios.c_cc[VERASE]*/'\b') {
		    if((int)pi > 0) {
			pi--;
			k = ((get_user_char((void *)(--data))
			    == '\t') ? TAB_SPACES : 1);
			do {
			    tty_echo(tty, ch);
			} while (--k);
		    }
		    continue;
		}
		if(/*(tty->termios.c_iflag & ICRNL) && */(ch == '\r'))
		    ch = '\n';

		if(ch == 04/*tty->termios.c_cc[VEOF]*/)
		    break;
	    }
	    put_user_char(ch, (void *)(data++));
	    tty_echo(tty, ch);
	    if((int)(++pi) >= len)
		break;
	} while((icanon && (ch != '\n') /*&& (ch != tty->termios.c_cc[VEOL])*/)
		    || (!icanon && (pi < tty->termios.c_cc[VMIN])));
    }
    return (int)pi;
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
