#ifndef LX86_LINUXMT_NTTY_H
#define LX86_LINUXMT_NTTY_H

/*
 * 'ntty.h' defines some structures used by ntty.c and some defines.
 */

#define INQ_SIZE 512
#define OUTQ_SIZE 64

/*
 * Note: don't mess with NR_PTYS until you understand the tty minor
 * number allocation game...
 * (Note: the *_driver.minor_start values 1, 64, 128, 192 are
 * hardcoded at present.)
 */

/* Predefined maximum number of tty character devices */

#define MAX_CONSOLES 3
#define MAX_SERIAL   4
#define MAX_PTYS     4

#define RS_MINOR_OFFSET 64

#if defined(CONFIG_CONSOLE_DIRECT) || defined(CONFIG_SIBO_CONSOLE_DIRECT) || defined(CONFIG_CONSOLE_BIOS)
#define NR_CONSOLES	MAX_CONSOLES
#else
#define NR_CONSOLES	0
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
};

struct tty {
    struct tty_ops *ops;
    unsigned short int minor;
    unsigned int flags;

#if 0
    struct wait_queue *sleep;
#endif

    struct ch_queue inq, outq;
    struct termios termios;
    int ostate;
    pid_t pgrp;
    unsigned char outq_buf[OUTQ_SIZE], inq_buf[INQ_SIZE];
};

extern int ttynull_openrelease(struct tty *);
		/* Empty function, returns zero. useful */

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

/* tty.flags */
#define TTY_STOPPED 	1
#define TTY_OPEN	2

#endif

#endif
