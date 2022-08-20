#ifndef __LINUXMT_NTTY_H
#define __LINUXMT_NTTY_H

/*
 * 'ntty.h' defines some structures used by ntty.c and some defines.
 */

/* NOTE: queue sizes no longer required to be power of two*/
#define INQ_SIZE	80	/* tty input queue size*/
#define OUTQ_SIZE	80	/* tty output queue size*/

#define PTYINQ_SIZE	80	/* pty input queue size*/
#define PTYOUTQ_SIZE	512	/* pty output queue size (=TDB_WRITE_MAX and telnetd buffer)*/

#define RSINQ_SIZE	1024	/* serial input queue SLIP_MTU+128+8*/
#define RSOUTQ_SIZE	80	/* serial output queue size*/

/*
 * Note: don't mess with NR_PTYS until you understand the tty minor
 * number allocation game...
 * (Note: the *_driver.minor_start values 0, 8, 64, 255 are hardcoded at present.)
 */

/* Predefined maximum number of tty character devices */

#define MAX_CONSOLES 3
#define MAX_PTYS     4

#define TTY_MINOR_OFFSET 0
#define PTY_MINOR_OFFSET 8
#define RS_MINOR_OFFSET 64

#if defined(CONFIG_CONSOLE_DIRECT) || defined(CONFIG_CONSOLE_BIOS)
#define NR_CONSOLES	MAX_CONSOLES
#else
#define NR_CONSOLES	1	/* headless*/
#endif

#ifdef CONFIG_PSEUDO_TTY
#define NR_PTYS		MAX_PTYS
#else
#define NR_PTYS		0
#endif

#ifdef CONFIG_CHAR_DEV_RS
#define NR_SERIAL	MAX_SERIAL
#else
#define NR_SERIAL	0
#endif

#define MAX_TTYS (NR_CONSOLES+NR_SERIAL+NR_PTYS)

#define DCGET_GRAPH	(('D'<<8)+0x01)
#define DCREL_GRAPH	(('D'<<8)+0x02)
#define DCSET_KRAW	(('D'<<8)+0x03)
#define DCREL_KRAW	(('D'<<8)+0x04)

#ifdef __KERNEL__

#include <linuxmt/types.h>
#include <linuxmt/fs.h>
#include <linuxmt/termios.h>
#include <linuxmt/chqueue.h>

#include <arch/system.h>

/* Most likely, not all of these will get used */

struct tty_ops {
    int (*open) ();
    void (*release) ();
    int (*write) ();
    int (*read) ();
    int (*ioctl) ();
    void (*conout) (dev_t, int);
};

struct tty {
    struct tty_ops *ops;
    unsigned short minor;
    unsigned int flags;
    struct ch_queue inq, outq;
    struct termios termios;
    unsigned char ostate;
    unsigned char usecount;
    pid_t pgrp;
};

extern int tty_intcheck(struct tty *,unsigned char);
		/* Check for ctrl-C etc.. */

extern int pipe_lseek(struct inode *,struct file *,off_t,int);
		/* Empty function, returns -ESPIPE. useful */

extern int tty_outproc(register struct tty *);
		/* TTY postprocessing */

extern struct termios def_vals;
		/* global use of def_vals */

struct tty *determine_tty(dev_t);
		/* Function to determine relevant tty */

extern int ttystd_open(struct tty *tty);
extern void ttystd_release(struct tty *tty);
extern int tty_allocq(struct tty *tty, int insize, int outsize);
extern void tty_freeq(struct tty *tty);
		/* Allocate and free character queues*/
extern void set_serial_irq(int tty, int irq);

extern void set_console(dev_t dev);

#ifdef CONFIG_CONSOLE_DIRECT
extern unsigned VideoSeg;
#endif

/* tty.flags */
#define TTY_STOPPED 	1
#define TTY_OPEN	2

#endif

#endif
