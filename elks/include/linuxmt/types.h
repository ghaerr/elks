#ifndef __LINUXMT_TYPES_H
#define __LINUXMT_TYPES_H

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
typedef __u32			sector_t;
typedef __u16			dev_t;
typedef __u16			flag_t;
typedef __u16			gid_t;

typedef __u32			u_ino_t;
#ifndef __KERNEL__
typedef u_ino_t			ino_t;
#else
#ifdef CONFIG_32BIT_INODES
typedef __u32			ino_t;
#else
typedef __u16			ino_t;
#endif
#endif /* __KERNEL__*/
typedef __u16			pid_t;
typedef __u16			uid_t;
typedef __u16			gid_t;

typedef __u16			mode_t;
typedef __u16			nlink_t;
typedef __u16			segext_t;
typedef __u16			uid_t;
typedef __u16			umode_t;

typedef __u8			cc_t;
typedef __u16			sig_t;

typedef __s16			sem_t;

/*@ignore@*/

/* The next three lines cause splint to complain needlessly */

typedef __u32			time_t;

/*@end@*/

typedef __u32			fd_mask_t;

#include <linuxmt/posix_types.h>

typedef __kernel_fd_set 	fd_set;

struct ustat {
    daddr_t			f_tfree;
    ino_t			f_tinode;
    char			f_fname[6];
    char			f_fpack[6];
};

#endif
