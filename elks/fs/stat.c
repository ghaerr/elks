/*
 *  linux/fs/stat.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/string.h>
#include <linuxmt/stat.h>
#include <linuxmt/fs.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>

#include <arch/segment.h>

static int cp_stat(inode,statbuf)
register struct inode * inode;
struct stat * statbuf;
{
	struct stat tmp;
/*	unsigned int blocks, indirect; UNUSED */

	memset(&tmp, 0, sizeof(tmp));
	tmp.st_dev = kdev_t_to_nr(inode->i_dev);
	tmp.st_ino = inode->i_ino;
	tmp.st_mode = inode->i_mode;
	tmp.st_nlink = inode->i_nlink;
	tmp.st_uid = inode->i_uid;
	tmp.st_gid = inode->i_gid;
	tmp.st_size = inode->i_size;
	tmp.st_rdev = kdev_t_to_nr(inode->i_rdev);
	tmp.st_mtime = inode->i_mtime;
#ifdef CONFIG_ACTIME
	tmp.st_atime = inode->i_atime;
	tmp.st_ctime = inode->i_ctime;
#else
	tmp.st_atime = inode->i_mtime;
	tmp.st_ctime = inode->i_mtime;
#endif
/*
 * st_blocks and st_blksize are approximated with a simple algorithm if
 * they aren't supported directly by the filesystem. The minix and msdos
 * filesystems don't keep track of blocks, so they would either have to
 * be counted explicitly (by delving into the file itself), or by using
 * this simple algorithm to get a reasonable (although not 100% accurate)
 * value.
 */

/*
 * Use minix fs values for the number of direct and indirect blocks.  The
 * count is now exact for the minix fs except that it counts zero blocks.
 * Everything is in BLOCK_SIZE'd units until the assignment to
 * tmp.st_blksize.
 */
#define D_B   7
#define I_B   (BLOCK_SIZE / sizeof(unsigned short))
/* This code does nothing useful. The results of the calculations below
 * are stored in local variables and nothing is done with them. 
 * Al
	if (!inode->i_blksize) {
		blocks = (tmp.st_size + BLOCK_SIZE - 1) >> BLOCK_SIZE_BITS;
		if (blocks > D_B) {
			indirect = (blocks - D_B + I_B - 1) / I_B;
			blocks += indirect;
			if (indirect > 1) {
				indirect = (indirect - 1 + I_B - 1) / I_B;
				blocks += indirect;
				if (indirect > 1)
					blocks++;
			}
		}
	}*/
	return verified_memcpy_tofs((char *)statbuf,(char *)&tmp,sizeof(tmp));
}

int sys_stat(filename,statbuf)
char * filename;
struct stat *statbuf;
{
	struct inode * inode;
	int error;

	error = namei(filename,&inode,0,0);
	if (!error) {
		error = cp_stat(inode,statbuf);
		iput(inode);
	}
	return error;
}

int sys_lstat(filename, statbuf)
char * filename;
struct stat * statbuf;
{
	struct inode * inode;
	int error;

	error = lnamei(filename,&inode);
	if (!error) {
		error = cp_stat(inode,statbuf);
		iput(inode);
	}
	return error;
}

int sys_fstat(fd, statbuf)
unsigned int fd;
struct stat * statbuf;
{
	struct file * f;
	int error;

	if ((error = fd_check(fd, (char *)statbuf, sizeof(struct stat), FMODE_WRITE | FMODE_READ, &f)) != 0) {
		return error;
	}
	return cp_stat(f->f_inode,statbuf);
}

int sys_readlink(path,buf,bufsiz)
char * path;
register char * buf;
int bufsiz;
{
	struct inode * inode;
	register struct inode_operations * iop;
	int error;

	if (bufsiz <= 0) {
		error = -EINVAL;
	} else {
		error = verify_area(VERIFY_WRITE,buf,bufsiz);
		if (!error) {
			error = lnamei(path,&inode);
			if (!error) {
				iop = inode->i_op;
				if (!iop || !iop->readlink) {
					iput(inode);
					error = -EINVAL;
				} else error = iop->readlink(inode,buf,bufsiz);
			}
		}
	}
	return error;
}
