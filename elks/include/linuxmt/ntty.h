#ifndef _LINUX_TTY_H
#define _LINUX_TTY_H

/*
 * 'tty.h' defines some structures used by tty_io.c and some defines.
 */

/*
 * These constants are also useful for user-level apps (e.g., VC
 * resizing).
 */
#define MIN_NR_CONSOLES	1	/* must be at least 1 */
#define MAX_NR_CONSOLES	1	/* serial lines start at 32 */
#define MAX_NR_USER_CONSOLES 1	/* must be root to allocate above this */

#define INQ_SIZE 160
#define OUTQ_SIZE 40

#define NUM_TTYS	12

#define DCGET_GRAPH	(('D'<<8)+0x01)
#define DCREL_GRAPH	(('D'<<8)+0x02)

#ifdef __KERNEL__
#include <linuxmt/fs.h>
#include <linuxmt/termios.h>
#include <linuxmt/clist.h>
#include <linuxmt/chqueue.h>

#include <arch/system.h>


/*
 * Note: don't mess with NR_PTYS until you understand the tty minor 
 * number allocation game...
 * (Note: the *_driver.minor_start values 1, 64, 128, 192 are
 * hardcoded at present.)
 */
 
#define NR_PTYS		4
#define NR_LDISCS	2

/* Not all of these will get used most likely */

struct tty_ops {
	int (*open)();
	int (*release)();
	int (*write)();
	int (*read)();
	int (*ioctl)();
};

struct tty {
	struct tty_ops *ops;
	int minor;
	int flags;
	unsigned char inq_buf[INQ_SIZE], outq_buf[OUTQ_SIZE];
/*	struct wait_queue *sleep; */
	struct ch_queue inq, outq;
	struct termios termios;
	pid_t pgrp;
};

int ttynull_openrelease();	/* Empty function, returns zero. useful */
extern int pipe_lseek();	/* Empty function, returns -ESPIPE. useful */

/* tty.flags */
#define TTY_STOPPED 	1
#define TTY_OPEN	2

#endif /* __KERNEL__ */
#endif
