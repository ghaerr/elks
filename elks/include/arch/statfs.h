#ifndef __ARCH_8086_STATFS_H__
#define __ARCH_8086_STATFS_H__

typedef struct {
	long    val[2];
} fsid_t;

struct statfs {
	long f_type;
	long f_bsize;
	long f_blocks;
	long f_bfree;
	long f_bavail;
	long f_files;
	long f_ffree;
	fsid_t f_fsid;
	long f_namelen;
	long f_spare[6];
};

#endif
