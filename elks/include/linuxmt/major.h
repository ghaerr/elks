#ifndef __LINUXMT_MAJOR_H
#define __LINUXMT_MAJOR_H

/*
 * This file has definitions for major device numbers
 */

/* limits */

#define MAX_CHRDEV 11
#define MAX_BLKDEV  7

/*
 * assignments
 *
 * devices are as follows (same as minix, so we can use the minix fs):
 *
 *      character              block                  comments
 *      --------------------   --------------------   --------------------
 *  0 - unnamed                unnamed                minor 0 = true nodev
 *  1 - /dev/mem               /dev/rd[01]            char mem, block ramdisk
 *  2 - /dev/ptyp*             /dev/ssd               char pty master, block ssd
 *  3 -                        /dev/{fd*,hd*}         block BIOS fd/hd
 *  4 - /dev/tty*,ttyp*,ttyS*  /dev/f{0,1}            char tty, pty slave, serial, block fd
 *  5 -
 *  6 - /dev/lp                /dev/rom               char lp, block romflash
 *  7 - /dev/ucd               /dev/ubd               meta UDD user device drivers
 *  8 - /dev/tcpdev                                   kernel <-> ktcp comm
 *  9 - /dev/eth                                      NIC driver
 * 10 - /dev/cgatext
 */

#define UNNAMED_MAJOR     0

/* These are the character devices */

#define MEM_MAJOR         1
#define PTY_MASTER_MAJOR  2
                             /* 3 unused*/
#define TTY_MAJOR         4
                             /* 5 unused*/
#define LP_MAJOR          6
#define UDD_MAJOR         7  /* experimental*/
#define TCPDEV_MAJOR      8
#define ETH_MAJOR         9
#define CGATEXT_MAJOR     10

/* These are the block devices */

#define RAM_MAJOR         1
#define SSD_MAJOR         2
#define BIOSHD_MAJOR      3
#define FLOPPY_MAJOR      4
#define ATHD_MAJOR        5  /* experimental*/
#define ROMFLASH_MAJOR    6

#endif
