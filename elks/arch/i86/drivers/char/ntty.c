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

#include <linuxmt/config.h>
#include <linuxmt/sched.h>
#include <linuxmt/fs.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/errno.h>
#include <linuxmt/mm.h>
#include <linuxmt/string.h>
#include <linuxmt/termios.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/ntty.h>
#include <linuxmt/major.h>
#include <linuxmt/kdev_t.h>
#include <linuxmt/init.h>
#include <linuxmt/debug.h>
#include <linuxmt/heap.h>
#include <arch/irq.h>

/* default termios, set at init time, not reset at open*/
struct termios def_vals = {
    BRKINT|ICRNL,                                       /* c_iflag*/
    OPOST|ONLCR,                                        /* c_oflag*/
    (tcflag_t) (B9600 | CS8),                           /* c_cflag*/
    (tcflag_t) (ISIG | ICANON | ECHO | ECHOE),          /* c_lflag*/
    0,                                                  /* c_line*/
    { 3,        /* VINTR*/
    28,         /* VQUIT*/
    8,          /* VERASE*/
    21,         /* VKILL unused*/
    4,          /* VEOF*/
    0,          /* VTIME*/
    1,          /* VMIN*/
    0,          /* VSWTCH unused*/
    17,         /* VSTART unused*/
    19,         /* VSTOP unused*/
    26,         /* VSUSP unused*/
    0,          /* VEOL*/
    18,         /* VREPRINT unused*/
    15,         /* VDISCARD unused*/
    23,         /* VWERASE unused*/
    22,         /* VLNEXT unused*/
    127         /* VERASE2*/
    }
};

#define TAB_SPACES 8

struct tty ttys[MAX_TTYS];

int tty_intcheck(register struct tty *ttyp, unsigned char key)
{
    sig_t sig = 0;

    if ((ttyp->termios.c_lflag & ISIG) && ttyp->pgrp) {
        if (key == ttyp->termios.c_cc[VINTR])
            sig = SIGINT;
        if (key == ttyp->termios.c_cc[VQUIT])
            sig = SIGQUIT;
        if (key == ttyp->termios.c_cc[VSUSP])
            sig = SIGTSTP;
#if DEBUG_EVENT
        if (key >= ('N' & 0x1f) && key <= ('P' & 0x1f)) {       /* CTRLN-CTRLP */
            debug_event((key - 'N') & 0x1f);
            return 1;
        }
#endif
        if (sig) {
            debug_tty("TTY signal %d to pgrp %d pid %P\n", sig, ttyp->pgrp);
            kill_pg(ttyp->pgrp, sig, 1);
        }
    }
    return sig;
}

/* Turns a dev_t variable into its tty, or NULL if it's not valid */

struct tty *determine_tty(dev_t dev)
{
    register struct tty *ttyp = &ttys[0];
    unsigned short minor = MINOR(dev);

    /* handle /dev/tty*/
    if (minor == 255 && current->pgrp && (current->pgrp == ttyp->pgrp))
        return current->tty;

    /* handle /dev/console*/
    if (minor == 254)
        return determine_tty(dev_console);

    do {
        if (ttyp->minor == minor)
            return ttyp;
    } while (++ttyp < &ttys[MAX_TTYS]);

    return NULL;
}

/* standard ops open/release routines for dircon and bioscon*/
int ttystd_open(struct tty *tty)
{
    /* increment use count, don't init if already open*/
    if (tty->usecount++)
        return 0;
    return tty_allocq(tty, INQ_SIZE, OUTQ_SIZE);
}

void ttystd_release(struct tty *tty)
{
    if (--tty->usecount == 0)
        tty_freeq(tty);
}

int tty_allocq(struct tty *tty, int insize, int outsize)
{
    if ((tty->inq.base = heap_alloc(insize, HEAP_TAG_TTY)) == NULL)
        return -ENOMEM;
    if ((tty->outq.base = heap_alloc(outsize, HEAP_TAG_TTY)) == NULL) {
        heap_free(tty->inq.base);
        return -ENOMEM;
    }
    chq_init(&tty->inq, tty->inq.base, insize);
    chq_init(&tty->outq, tty->outq.base, outsize);
    return 0;
}

void tty_freeq(struct tty *tty)
{
    heap_free(tty->inq.base);
    heap_free(tty->outq.base);
}

int tty_open(struct inode *inode, struct file *file)
{
    struct tty *otty;
    int err;

    if (!(otty = determine_tty(inode->i_rdev)))
        return -ENODEV;

    debug_tty("TTY open pid %P\n");
#if UNUSED
    memcpy(&otty->termios, &def_vals, sizeof(struct termios));
#endif

    if ((file->f_flags & O_EXCL) && (otty->flags & TTY_OPEN))
        return -EBUSY;

    /* don't call driver on /dev/tty open*/
    if (MINOR(inode->i_rdev) == 255)
        return 0;

    err = otty->ops->open(otty);
    if (!err) {
        if (!(file->f_flags & O_NOCTTY) && current->session == current->pid
                && current->tty == NULL && otty->pgrp == 0) {
            debug_tty("TTY setting pgrp %d pid %P\n", current->pgrp);
            otty->pgrp = current->pgrp;
            current->tty = otty;
        }
        otty->flags |= TTY_OPEN;
    }
    return err;
}

void tty_release(struct inode *inode, struct file *file)
{
    register struct tty *rtty;

    rtty = determine_tty(inode->i_rdev);
    if (!rtty)
        return;

    debug_tty("TTY close pid %P\n");

    /* no action if closing /dev/tty*/
    if (MINOR(inode->i_rdev) == 255)
        return;

    /* don't release pgrp for /dev/tty, only real tty*/
    if (current->pid == rtty->pgrp) {
        debug_tty("TTY release pgrp %P\n");
        if ((int)rtty->termios.c_cflag & HUPCL) {       /* warning truncated to 16 bits*/
                debug_tty("TTY sending SIGHUP pid %P\n");
                kill_pg(rtty->pgrp, SIGHUP, 1);
        }
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
 * while (tty->outq.len > 0) {
 *      ch = tty_outproc(tty);
 *      write_char_to_device(device, ch);
 *      cnt++;
 * }
 *
 */
int tty_outproc(register struct tty *tty)
{
    int t_oflag;    /* WARNING: termios.c_oflag truncated to 16 bits */
    int ch;

    ch = tty->outq.base[tty->outq.tail];
    if ((t_oflag = (int)tty->termios.c_oflag) & OPOST) {
        switch((int)(tty->ostate & ~0x0F)) {
        case 0x20:              /* Expand tabs to spaces */
            ch = ' ';
            if (--tty->ostate & 0xF)
                break;
        case 0x10:              /* Expand NL -> CR NL */
            tty->ostate = 0;
            break;
        default:
            switch(ch) {
            case '\n':
                if (t_oflag & ONLCR) {
                    ch = '\r';                          /* Expand NL -> CR NL */
                    tty->ostate = 0x10;
                }
                break;
#if UNUSED
            case '\r':
                if (t_oflag & OCRNL)
                    ch = '\n';                          /* Map CR to NL */
                break;
#endif
            case '\t':
                if ((t_oflag & TABDLY) == XTABS) {
                    ch = ' ';                           /* Expand tabs to spaces */
                    tty->ostate = 0x20 + TAB_SPACES - 1;
                }
            }
        }
    }
    if (!tty->ostate) {
        if (++tty->outq.tail >= tty->outq.size)
            tty->outq.tail = 0;
        tty->outq.len--;
    }
    return ch;
}

static void tty_echo(register struct tty *tty, unsigned char ch)
{
    if ((tty->termios.c_lflag & ECHO)
                || ((tty->termios.c_lflag & ECHONL) && (ch == '\n'))) {
        if ((ch == tty->termios.c_cc[VERASE] || ch == tty->termios.c_cc[VERASE2])
                && (tty->termios.c_lflag & ECHOE)) {
            chq_addch(&tty->outq, '\b');
            chq_addch(&tty->outq, ' ');
            chq_addch(&tty->outq, '\b');
        } else
            chq_addch(&tty->outq, ch);
        tty->ops->write(tty);
    }
}

/*
 *      Write to a terminal
 *
 */

size_t tty_write(struct inode *inode, struct file *file, char *data, size_t len)
{
    register struct tty *tty = determine_tty(inode->i_rdev);
    size_t i;
    int s;

    i = 0;
    while (i < len) {
        s = chq_wait_wr(&tty->outq, (file->f_flags & O_NONBLOCK) | i);
        if (s < 0) {
            /* FIXME EAGAIN not returned, cycle required on telnet nonblocking terminal */
            if (s == -EINTR || s == -EAGAIN) {
                tty->ops->write(tty);
                wake_up(&tty->outq.wait);
                schedule();
                continue;
            }
            if (i == 0)
                i = s;
            break;
        }
        chq_addch_nowakeup(&tty->outq, get_user_char(data++));
        i++;
    }
    tty->ops->write(tty);
    wake_up(&tty->outq.wait);
    return i;
}

size_t tty_read(struct inode *inode, struct file *file, char *data, size_t len)
{
    register struct tty *tty = determine_tty(inode->i_rdev);
    int icanon = tty->termios.c_lflag & ICANON;
    unsigned int vmin = tty->termios.c_cc[VMIN];
    unsigned int vtime = tty->termios.c_cc[VTIME];
    int nonblock = (file->f_flags & O_NONBLOCK) || (!icanon && vtime && !vmin);
    jiff_t timeout;
    size_t i = 0;
    int ch, k;

    while (i < len) {
        timeout = jiffies + vtime * (HZ / 10);
again:
        if (tty->ops->read) {
            tty->ops->read(tty);
            nonblock = 1;
        }

        if (chq_peekch(&tty->inq))
            ch = chq_getch(&tty->inq);
        else {
            if (!icanon && !vtime && (i >= vmin))
                break;
            ch = chq_wait_rd(&tty->inq, nonblock);
            if (ch < 0) {
                if (!icanon && vtime) {
                    if (jiffies < timeout) {
                        schedule();
                        goto again;             /* don't reset timer*/
                    } else {
                        if (vmin && (i == 0)) {
                            nonblock = 1;
                            continue;           /* VMIN > 0 reset timer*/
                        }
                    }
                    ch = 0;     /* return 0 on VTIME timeout*/
                }
                if (i == 0)
                    i = ch;
                break;
            }
            ch = chq_getch(&tty->inq);
        }

        if ((tty->termios.c_iflag & ICRNL) && (ch == '\r'))
            ch = '\n';

        if (icanon) {
            if (ch == tty->termios.c_cc[VERASE] || ch == tty->termios.c_cc[VERASE2]) {
                if (i > 0) {
                    i--;
                    k = ((get_user_char((void *)(--data)) == '\t') ? TAB_SPACES : 1);
                    do {
                        tty_echo(tty, ch);
                    } while (--k);
                }
                continue;
            }

            if (ch == tty->termios.c_cc[VEOF])
                break;
        } else {
            if (vtime && vmin)  /* start timeout after first character*/
                nonblock = 1;
        }
        put_user_char(ch, (void *)(data++));
        tty_echo(tty, ch);
        i++;
        if (icanon && ((ch == '\n') || (ch == tty->termios.c_cc[VEOL])))
            break;
    }
    return i;
}

int tty_ioctl(struct inode *inode, struct file *file, int cmd, char *arg)
{
    register struct tty *tty = determine_tty(inode->i_rdev);
    int ret, dev;

    switch (cmd) {
    case TCGETS:
        ret = verified_memcpy_tofs(arg, &tty->termios, sizeof(struct termios));
        break;
    case TCSETS:
    case TCSETSW:
    case TCSETSF:
        ret = verified_memcpy_fromfs(&tty->termios, arg, sizeof(struct termios));

        /* Inform subdriver of new settings*/
        if (ret == 0 && tty->ops->ioctl != NULL)
            ret = tty->ops->ioctl(tty, cmd, arg);
        break;
    case TIOSETCONSOLE:
        dev = get_user(arg);
        if (MAJOR(dev) != TTY_MAJOR)
            return -EINVAL;
        set_console(dev);
        return 0;
    default:
        ret = ((tty->ops->ioctl == NULL)
            ? -EINVAL
            : tty->ops->ioctl(tty, cmd, arg));
    }
    return ret;
}

int tty_select(struct inode *inode, struct file *file, int sel_type)
{
    register struct tty *tty = determine_tty(inode->i_rdev);
    int ret = 0;

    switch (sel_type) {
    case SEL_IN:
        if (tty->inq.len != 0)
            return 1;
        select_wait(&tty->inq.wait);
        break;
    case SEL_OUT:
        ret = (tty->outq.len != tty->outq.size);
        if (!ret)
            select_wait(&tty->outq.wait);
    }
    return ret;
}

/*
 * Busy wait for a keypress in kernel state
 * Only used in mount_root for changing floppies
 */
int wait_for_keypress(void)
{
    set_irq();
    return chq_wait_rd(&ttys[0].inq, 0);
}

static struct file_operations tty_fops = {
    pipe_lseek,                 /* Same behavoir, return -ESPIPE */
    tty_read,
    tty_write,
    NULL,
    tty_select,
    tty_ioctl,
    tty_open,
    tty_release
};

/* TTY subdrivers, linked in via configuration*/
extern struct tty_ops dircon_ops;       /* CONFIG_CONSOLE_DIRECT*/
extern struct tty_ops bioscon_ops;      /* CONFIG_CONSOLE_BIOS*/
extern struct tty_ops headlesscon_ops;  /* CONFIG_CONSOLE_HEADLESS*/
extern struct tty_ops rs_ops;           /* CONFIG_CHAR_DEV_RS*/
extern struct tty_ops ttyp_ops;         /* CONFIG_PSEUDO_TTY*/
extern struct tty_ops i8018xcon_ops;    /* CONFIG_CONSOLE_8018X*/

void INITPROC tty_init(void)
{
    register struct tty *ttyp;
    int i;

    ttyp = &ttys[MAX_TTYS];
    do {
        ttyp--;
        ttyp->minor = (unsigned short int)(-1L); /* set unsigned to -1 */
        memcpy(&ttyp->termios, &def_vals, sizeof(struct termios));
    } while (ttyp > ttys);

    for (i = TTY_MINOR_OFFSET ; i < NR_CONSOLES + TTY_MINOR_OFFSET ; i++) {
#if defined(CONFIG_CONSOLE_DIRECT)
        ttyp->ops = &dircon_ops;
#elif defined(CONFIG_CONSOLE_BIOS)
        ttyp->ops = &bioscon_ops;
#elif defined(CONFIG_CONSOLE_8018X)
        ttyp->ops = &i8018xcon_ops;
#else
        ttyp->ops = &headlesscon_ops;
#endif
        (ttyp++)->minor = i;
    }

#ifdef CONFIG_CHAR_DEV_RS
    /* put serial entries after console entries */
    for (i = RS_MINOR_OFFSET; i < NR_SERIAL + RS_MINOR_OFFSET; i++) {
        ttyp->ops = &rs_ops;
        (ttyp++)->minor = i;            /* ttyS0 = RS_MINOR_OFFSET */
    }
#endif

#ifdef CONFIG_PSEUDO_TTY
    /* start at minor = 8 fixed to match pty entries in MAKEDEV */
    /* put slave pseudo tty entries after serial entries */
    for (i = PTY_MINOR_OFFSET; (i) < NR_PTYS + PTY_MINOR_OFFSET; i++) {
        ttyp->ops = &ttyp_ops;
        (ttyp++)->minor = i;            /* ttyp0 = PTY slave PTY_MINOR_OFFSET */
    }
#endif

    register_chrdev(TTY_MAJOR, "tty", &tty_fops);
}
