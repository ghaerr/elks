#ifndef _LINUX_TTY_H
#define _LINUX_TTY_H

/*
 * 'ntty.h' defines some structures used by ntty.c and some defines.
 */

#define INQ_SIZE 512
#define OUTQ_SIZE 64

#define NUM_TTYS	6

#define DCGET_GRAPH	(('D'<<8)+0x01)
#define DCREL_GRAPH	(('D'<<8)+0x02)
#define DCSET_KRAW	(('D'<<8)+0x03)
#define DCREL_KRAW	(('D'<<8)+0x04)

#ifdef __KERNEL__
#include <linuxmt/fs.h>
#include <linuxmt/termios.h>
#include <linuxmt/chqueue.h>

#include <arch/system.h>


/*
 * Note: don't mess with NR_PTYS until you understand the tty minor 
 * number allocation game...
 * (Note: the *_driver.minor_start values 1, 64, 128, 192 are
 * hardcoded at present.)
 */
 
#define NR_PTYS		4

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
int tty_intcheck();		/* Check for ctrl-C etc.. */
extern int pipe_lseek();	/* Empty function, returns -ESPIPE. useful */

extern struct termios def_vals; /* global use of def_vals                 */

/* tty.flags */
#define TTY_STOPPED 	1
#define TTY_OPEN	2

#endif /* __KERNEL__ */
#endif
