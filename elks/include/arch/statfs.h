#ifndef __ARCH_8086_STATFS_H
#define __ARCH_8086_STATFS_H

#define MNAMELEN	32	/* length of buffer for returned name */

/* ustatfs flags */
#define UF_NOFREESPACE      1   /* don't calculate time-expensive free blocks */

struct statfs {
	int	f_type;		/* filesystem type */
	unsigned short f_flags;	/* copy of mount flags */
	dev_t	f_dev;		/* device number */
	long	f_bsize;	/* sector size */
	long	f_blocks;	/* total 1K blocks in filesystem */
	long	f_bfree;	/* free 1K blocks in filesystem */
	long	f_bavail;	/* free 1K blocks available to non-superuser */
	long	f_files;	/* total file nodes (inodes) in filesystem */
	long	f_ffree;	/* free file nodes in filesystem */
	char	f_mntonname[MNAMELEN]; /* directory on which mounted */
};

#endif
