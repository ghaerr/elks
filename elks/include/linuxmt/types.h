// This is the actual "sys/types.h"

#pragma once

#include <arch/types.h>
#include <linuxmt/config.h>


typedef __s32			loff_t;
typedef __s32			off_t;

typedef __u32			daddr_t;
typedef __u32			jiff_t;
typedef __u32			lflag_t;
typedef __u32			lsize_t;
typedef __u32			speed_t;
typedef __u32			tcflag_t;

typedef __u16			block_t;
typedef __u32			block32_t;
typedef __u16			dev_t;
typedef __u16			flag_t;
typedef __u16			gid_t;

// Large FAT volumes need 32-bit inode number
// to store both sector number and directory entry relative index

// PRIino is C99-like macro for portability

#ifdef CONFIG_32BIT_INODES
typedef __u32 ino_t;
#define PRIino "lu"
#else
typedef __u16 ino_t;
#define PRIino "u"
#endif

typedef __u16			mode_t;
typedef __u16			nlink_t;
typedef __u16			segext_t;
typedef __u16			uid_t;
typedef __u16			umode_t;

typedef __u8			cc_t;
#ifndef sig_t
typedef __u8			sig_t;
#endif

/*@ignore@*/

/* The next three lines cause splint to complain needlessly */

typedef __u32			time_t;

/*@end@*/

#ifdef CONFIG_SHORT_FILES
typedef __u16			fd_mask_t;
#else
typedef __u32			fd_mask_t;
#endif

#include <linuxmt/config.h>
#include <linuxmt/posix_types.h>

typedef __kernel_fd_set 	fd_set;

#define PRINT_STATS(a) {					\
	int __counter;						\
	printk("Entering %s : (%x, %x)\n",			\
		a, current->t_regs.sp, current->t_kstack);	\
}

struct ustat {
    daddr_t			f_tfree;
    ino_t			f_tinode;
    char			f_fname[6];
    char			f_fpack[6];
};
