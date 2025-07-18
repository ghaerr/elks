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
 *      character              block               char          block
 *      -----------------   --------------------   ------------- ------------
 *  0 - nodevice            nodevice
 *  1 - /dev/mem            /dev/rd[01]            mem           ramdisk
 *  2 - /dev/ptyp[0123]     /dev/ssd               pty master    XMS ramdisk or SSD
 *  3 -                     /dev/{fd[01],hd[ab][1-7]}            BIOS floppy/hd
 *  4 - /dev/tty[1234]      /dev/df[01]            tty           direct floppy
 *  4 - /dev/ttyp[0123]                            pty slave
 *  4 - /dev/ttyS[0123]                            serial
 *  5 -                     /dev/cf[ab][1-7]                     direct ATA/CF/hd
 *  6 - /dev/lp             /dev/rom               printer       rom filesystem
 *  7 -
 *  8 - /dev/tcpdev                                kernel <-> ktcp
 *  9 - /dev/eth                                   NIC driver
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
                             /* 7 unused*/
#define TCPDEV_MAJOR      8
#define ETH_MAJOR         9
#define CGATEXT_MAJOR     10

/* These are the block devices */

#define RAM_MAJOR         1
#define SSD_MAJOR         2
#define BIOSHD_MAJOR      3
#define FLOPPY_MAJOR      4  /* direct floppy driver directfd.c */
#define ATHD_MAJOR        5  /* direct ATA/CF driver and nonworking directhd.c */
#define ROMFLASH_MAJOR    6

#endif
