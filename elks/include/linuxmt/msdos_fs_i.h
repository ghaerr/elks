#ifndef _MSDOS_FS_I
#define _MSDOS_FS_I

/*
 * MS-DOS file system inode data in memory
 * ported from linux-2.0.34 by zouys, Oct 28th, 2010
 */

struct msdos_inode_info {
	cluster_t i_start; /* first cluster or 0 */
	int i_attrs;	/* unused attribute bits */
	int i_busy;	/* file is either deleted but still open, or
			   inconsistent (mkdir) */
};

#endif
