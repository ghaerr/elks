#ifndef _LINUX_MAJOR_H
#define _LINUX_MAJOR_H

/*
 * This file has definitions for major device numbers
 */

/* limits */

#define MAX_CHRDEV 8
#define MAX_BLKDEV 8

/*
 * assignments
 *
 * devices are as follows (same as minix, so we can use the minix fs):
 *
 *      character              block                  comments
 *      --------------------   --------------------   --------------------
 *  0 - unnamed                unnamed                minor 0 = true nodev
 *  1 - /dev/mem               ramdisk
 *  2 - /dev/ptyp*             floppy
 *  3 - /dev/ttyp*             bioshd
 *  4 - /dev/tty*	       mscdex
 *  5 - /dev/tty; /dev/cua*    athd
 *  6 - lp
 *  7 - mice
 */

#define UNNAMED_MAJOR	0
#define MEM_MAJOR	1
#define PTY_MASTER_MAJOR 2
#define PTY_SLAVE_MAJOR 3
#define TTY_MAJOR	4
#define TTYAUX_MAJOR	5
#define LP_MAJOR	6
#define MISC_MAJOR	7

#define RAM_MAJOR	1
#define FLOPPY_MAJOR	2
#define BIOSHD_MAJOR	3
#define MSCDEX_MAJOR	4
#define ATHD_MAJOR	5
#endif
