#ifndef __ARCH_8086_STATFS_H
#define __ARCH_8086_STATFS_H

#define MNAMELEN	32	/* length of buffer for returned name */

struct statfs {
	int	f_type;		/* filesystem type */
	unsigned short f_flags;	/* copy of mount flags */
	long	f_bsize;	/* block (or sector) size */
	long	f_blocks;	/* total data blocks in filesystem */
	long	f_bfree;	/* free data blocks in filesystem */
	long	f_bavail;	/* free blocks available to non-superuser */
	long	f_files;	/* total file nodes (inodes) in filesystem */
	long	f_ffree;	/* free file nodes in filesystem */
	char	f_mntonname[MNAMELEN]; /* directory on which mounted */
};

#endif
