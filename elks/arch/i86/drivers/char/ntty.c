/*
 * arch/i86/drivers/char/tty.c - A simple tty driver
 * (C) 1997 Chad Page et. al
 *
 * Modified by Greg Haerr <greg@censoft.com> for screen editors
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
#include <linuxmt/major.h>

/*
 * XXX plac: setting default to B9600 instead of B1200
 */
struct termios def_vals = { 	BRKINT,
				ONLCR,
				B9600 | CS8,
				ECHO | ICANON | ISIG,
				0,
				{3,28,127,21,4,0,1,0,17,19,26,0,18,15,23,22}
			};


#define TAB_SPACES 8

#define MAX_TTYS NUM_TTYS
struct tty ttys[MAX_TTYS];
extern struct tty_ops dircon_ops;
extern struct tty_ops bioscon_ops;
extern struct tty_ops rs_ops;
extern struct tty_ops ttyp_ops;

int tty_intcheck(ttyp, key)
register struct tty * ttyp;
unsigned char key;
{
	if ((ttyp->termios.c_lflag & ISIG) && (ttyp->pgrp)) {
		int sig = 0;
		if (key == ttyp->termios.c_cc[VINTR]) {
			sig = SIGINT;
		}
		if (key == ttyp->termios.c_cc[VSUSP]) {
			sig = SIGTSTP;
		}
		if (sig) {
			kill_pg(ttyp->pgrp, sig, 1);
			return 1;
		}
	}
	return 0;
}

/* Turns a dev_t variable into its tty, or NULL if it's not valid */

struct tty * determine_tty(dev)
dev_t dev;
{
	int i, minor = MINOR(dev);
	register struct tty * ttyp;

	for (i = 0; i < MAX_TTYS; i++) {
		ttyp = &ttys[i];
		if (ttyp->minor == minor)
			return ttyp;
	}
	return 0; 
}

int tty_open(inode, file)
register struct inode * inode;
struct file *file;
{
	register struct tty * otty;
	int err;

	if (otty = determine_tty(inode->i_rdev)) {
		memcpy(&otty->termios, &def_vals, sizeof(struct termios));
		err = otty->ops->open(otty);
		if (err) {
			return err;
		}
		if (otty->pgrp == NULL && current->session == current->pid
		    && current->tty == NULL) {
			otty->pgrp = current->pgrp;
			current->tty = otty;
		}
		otty->flags |= TTY_OPEN;
		chq_init(&otty->inq, otty->inq_buf, INQ_SIZE);
		chq_init(&otty->outq, otty->outq_buf, OUTQ_SIZE);
		return 0;
	} else {
		return -ENODEV;
	}

}

int ttynull_openrelease(tty)
struct tty * tty;
{
	return 0;
}

int tty_release(inode,file)
register struct inode *inode;
struct file *file;
{
	register struct tty *rtty;
	rtty = determine_tty(inode->i_rdev);
	if (rtty)  {
		if (current->pid == rtty->pgrp) {
			kill_pg(rtty->pgrp, SIGHUP, 1);
			rtty->pgrp = NULL;
		}
		rtty->flags &= ~TTY_OPEN;
		return rtty->ops->release(rtty);
	} else {
		return -ENODEV;
	}
}

/* Write 1 byte to a terminal, with processing */

void tty_charout(tty, ch)
register struct tty *tty;
unsigned char ch;
{
	int j;

	switch (ch) {
		case '\t':		
			for (j = 0; j < TAB_SPACES; j++) {
				tty_charout(tty, ' ');
			}
			break;
		case '\n':
			if (tty->termios.c_oflag & ONLCR) {
				tty_charout(tty, '\r');
			}
			/* fall through */
		default:
			while (chq_addch(&tty->outq, ch, 0) == -1) {
				tty->ops->write(tty);
			}
	};
}

void tty_echo(tty, ch)
register struct tty *tty;
unsigned char ch;
{
	if (tty->termios.c_lflag & ECHO) {
		tty_charout(tty, ch);
	}
}

/*
 *	Write to a terminal
 *
 */
 
int tty_write(inode,file,data,len)
struct inode *inode;
struct file *file;
char *data;
int len;
{
	register struct tty *tty=determine_tty(inode->i_rdev);
	int i = 0;

	while (i < len) {
		tty_charout(tty, peekb(current->t_regs.ds, data + i++)); 
	}
	tty->ops->write(tty);
	return i;
}

int tty_read(inode,file,data,len)
struct inode *inode;
struct file *file;
char *data;
int len;
{
	register struct tty *tty=determine_tty(inode->i_rdev);
	int i = 0, j = 0, k;
	unsigned char ch, lch;
	int rawmode = (tty->termios.c_lflag & ICANON)? 0: 1;
	int blocking = (file->f_flags & O_NONBLOCK)? 0 : 1;

	if(len == 0)
		return 0;
	do {
		if (tty->ops->read) {
			tty->ops->read(tty);
			blocking = 0;
		}
		j = chq_getch(&tty->inq, &ch, blocking);
		if (j == -1) {
			if (blocking) {
				return -EINTR;
			} else {
				break;
			}
		}
		if (!rawmode && (j == 04))	/* CTRLD*/
			break;
		if (rawmode || (j != '\b')) {
			pokeb(current->t_regs.ds, (data + i++), ch);		
			tty_echo(tty, ch);
		} else if (i > 0) {
			lch = ((peekb(current->t_regs.ds, (data + --i)) == '\t') ? TAB_SPACES : 1 );
			for (k = 0; k < lch ; k++) {
				tty_echo(tty, ch);
			}
		}
		tty->ops->write(tty);
	} while(i < len && !rawmode && j != '\n');
	/*} while(i < len && (rawmode || j != '\n'));*/

	return i;
}

int tty_ioctl(inode,file, cmd, arg)
struct inode *inode;
struct file *file;
int cmd;
char *arg;
{
	register struct tty *tty=determine_tty(inode->i_rdev);
	int retval;

	switch (cmd) {
		case TCGETS:
			return verified_memcpy_tofs(arg, &tty->termios, sizeof (struct termios));
			
			break;
		case TCSETS:
		case TCSETSW:
		case TCSETSF:
			retval = verified_memcpy_fromfs(&tty->termios, arg, sizeof(struct termios));
			/* Inform driver that things have changed */
			if (tty->ops->ioctl != NULL) {
				tty->ops->ioctl(tty, cmd, arg);
			}
			return retval;
			break;
		default:
			if (tty->ops->ioctl == NULL) {
				return -EINVAL;
			} else {
				return tty->ops->ioctl(tty, cmd, arg);
			}
	}
	return 0;
}

#if 0 /* This is the same bahavoir as pipe lseek, so we use that. */
int tty_lseek(inode,file,offset,origin)
struct inode *inode;
struct file *file;
off_t offset;
int origin;
{
	return -ESPIPE;
}
#endif

#if 0 /* Default returns -ENOTDIR if no readdir exists, so this is redundant */
int tty_readdir()
{
	return -ENOTDIR;
}
#endif

int tty_select (inode, file, sel_type, wait)
struct inode * inode; /* how revolting, K&R style defs */
struct file * file;
int sel_type;
select_table * wait;
{
	register struct tty *tty=determine_tty(inode->i_rdev);

	switch (sel_type) {
		case SEL_IN:
			if (chq_peekch(&tty->inq)) {
				return 1;
			}
		case SEL_EX: /* fall thru! */
			if (wait) {
				select_wait (&tty->inq.wq, wait);
			}
			return 0;
		case SEL_OUT: /* Hm.  We can always write to a tty?  (not really) */
		default:
			return 1;
	}
}

static struct file_operations tty_fops =
{
	pipe_lseek,	/* Same behavoir, return -ESPIPE */
	tty_read,
	tty_write,
	NULL,
	tty_select,	/* Select - needs doing */
	tty_ioctl,		/* ioctl */
	tty_open,
	tty_release,
#ifdef BLOAT_FS
	NULL,
	NULL,
	NULL
#endif
};

void tty_init()
{
	int i;
	register struct tty * ttyp;

	for (i = 0; i < NUM_TTYS; i++) { 
		ttyp = &ttys[i];
		ttyp->minor = -1;
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
		ttyp->minor = i;
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
	register_chrdev(TTY_MAJOR,"tty",&tty_fops);
}

