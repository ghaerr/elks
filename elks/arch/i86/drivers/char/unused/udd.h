/*
 * include/linuxmt/udd.h
 *
 * Copyright (C) 1999 by Alistair Riddoch
 *
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL)
 *
 * Header file for the ELKS meta driver for user space device drivers
 */

#ifndef __LINUXMT_UDD_H
#define __LINUXMT_UDD_H

#include <linuxmt/fs.h>

#define MAX_UDD	1 /* FIXME only 1 UDD until device passed to request_fn*/
#define MAX_UDR	8

struct ud_driver {
    int udd_type;
    int udd_major;
    struct task_struct *udd_task;
    char *udd_data;
    struct ud_request *udd_req;
    struct wait_queue udd_wait;
    struct wait_queue udd_rwait;
};

/*
 * This is the truncated version of the above that we copy back from user
 * space, so as to guaranteee not everwriting kernel pointers and stuff.
 * Must be the same order, but shorted and with no gaps, or kernel pointers
 * in it.
 */

struct ud_driver_trunc {
    int udd_type;
    int udd_major;
    struct task_struct *udd_task;
    char *udd_data;
};

/* udd_type can be */
#define UDD_NONE	0
#define UDD_CHR_DEV	1
#define UDD_BLK_DEV	2

struct ud_request {
    char *udr_data;
    int udr_status;
    off_t udr_ptr;
    int udr_param;
    struct wait_queue udr_wait;
    struct wait_queue udr_ewait;
    int udr_type;
    int udr_minor;
    unsigned int udr_size;
};

/*
 * This is the truncated version of the above that we copy back from user
 * space, so as to guaranteee not everwriting kernel pointers and stuff.
 * Must be the same order, but shorted and with no gaps, or kernel pointers
 * in it.
 */

struct ud_request_trunc {
    char *udr_data;
    int udr_status;
};

/* udr_type can be */
#define UDR_LSEEK	0
#define UDR_READ	1
#define UDR_WRITE	2
#define UDR_SELECT	3
#define UDR_IOCTL	4
#define UDR_OPEN	5
#define UDR_RELEASE	6
#define UDR_BLK		7
#define UDR_BLK_READ	(UDR_BLK + READ)
#define UDR_BLK_WRITE	(UDR_BLK + WRITE)

/* ioctls */

#define META_BASE	0x0700
#define	META_CREAT	0x0701
#define	META_POLL	0x0702
#define	META_ACK	0x0703

#endif
