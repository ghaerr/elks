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

#ifndef __LINUXMT_UDD_H_
#define __LINUXMT_UDD_H_

#define MAX_UDD	8
#define MAX_UDR	32

struct ud_driver {
	int	udd_type;
	char *	udd_name;
	int	udd_first_minor;
	int	udd_num_minors;
	pid_t	udd_pid;
	struct task_struct * udd_task;
};

/* udd_type can be */
#define UDD_NONE	0
#define UDD_CHR_DEV	1
#define UDD_BLK_DEV	2

struct ud_request {
	int	udr_type;
	int	udr_no;
	char *	udr_data;
	unsigned int	udr_size;
	unsigned long	udr_ptr;
	int	udr_param;
};

/* udr_type can be */
#define UDR_LSEEK	0
#define UDR_READ	1
#define UDR_WRITE	2
#define UDR_SELECT	3
#define UDR_IOCTL	4
#define UDR_OPEN	5
#define UDR_RELEASE	5

	

#endif /* __LINUXMT_UDD_H_ */
