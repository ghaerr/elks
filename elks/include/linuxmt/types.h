#ifndef __LINUXMT_TYPES_H
#define __LINUXMT_TYPES_H

#include <arch/types.h>
#include <linuxmt/posix_types.h>
#include <linuxmt/config.h>

#define PRINT_STATS(a) {					\
	int __counter;						\
	printk("Entering %s : (%x, %x)\n",			\
		a, current->t_regs.sp, current->t_kstack);	\
}

/* Throw away _FUNCTION parameters - the syntax is ripped off of Minix's
   _PROTOTYPE.  Considering Borland did the same thing to MFC on a bigger
   scale, I don't think PH will mind :) */

/* Yes, this should be in arch/types.h too */

#define _FUNCTION(function, params) function()
#define _VFUNCTION(functiom, params) (*function) ()

typedef __s32 off_t;
typedef __s16 pid_t;
typedef __u16 uid_t;
typedef __u16 gid_t;
typedef __u16 dev_t;
typedef __u32 time_t;
typedef __u16 umode_t;
typedef __u16 nlink_t;
typedef __u16 mode_t;
typedef __u32 loff_t;
typedef __u32 speed_t;
typedef __u16 size_t;
typedef __u32 lsize_t;
typedef __u16 ino_t;
typedef __u32 u_ino_t;
typedef __u16 block_t;
typedef __u32 daddr_t;
typedef __u32 tcflag_t;
typedef __u8  cc_t;
typedef __kernel_fd_set fd_set;
typedef __u16 seg_t;			/* Segment no. */
typedef __u16 segext_t;			/* Extent of segment */
typedef __u32 jiff_t;
#ifdef CONFIG_SHORT_FILES
typedef __u16 fd_mask_t;
#else /* CONFIG_SHORT_FILES */
typedef __u32 fd_mask_t; 
#endif /* CONFIG_SHORT_FILES */

typedef int   ptrdiff_t;
struct ustat {
	daddr_t f_tfree;
	ino_t f_tinode;
	char f_fname[6];
	char f_fpack[6];
};

#endif

