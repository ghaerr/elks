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

static int cp_stat(register struct inode *inode, struct stat *statbuf)
{
    static struct stat tmp;		/* static not reentrant: conserve stack usage*/

    memset(&tmp, 0, sizeof(tmp));

    tmp.st_dev		= kdev_t_to_nr(inode->i_dev);
    tmp.st_ino		= inode->i_ino;
    tmp.st_mode 	= inode->i_mode;
    tmp.st_nlink	= inode->i_nlink;
    tmp.st_uid		= inode->i_uid;
    tmp.st_gid		= inode->i_gid;
    tmp.st_size 	= (off_t) inode->i_size;
    tmp.st_rdev 	= kdev_t_to_nr(inode->i_rdev);
    tmp.st_mtime	= inode->i_mtime;
    tmp.st_atime	= inode->i_atime;
    tmp.st_ctime	= inode->i_ctime;

#if UNUSED
/*
 * st_blocks and st_blksize are approximated with a simple algorithm if
 * they aren't supported directly by the filesystem. The minix and msdos
 * filesystems don't keep track of blocks, so they would either have to
 * be counted explicitly (by delving into the file itself), or by using
 * this simple algorithm to get a reasonable (although not 100% accurate)
 * value.
 *
 * Use minix fs values for the number of direct and indirect blocks.  The
 * count is now exact for the minix fs except that it counts zero blocks.
 * Everything is in BLOCK_SIZE'd units until the assignment to
 * tmp.st_blksize.
 *
 * This code does nothing useful. The results of the calculations below
 * are stored in local variables and nothing is done with them.
 * Al
 */
#define D_B   7
#define I_B   (BLOCK_SIZE / sizeof(unsigned short))

    if (!inode->i_blksize) {
	unsigned int blocks, indirect;

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
    }
#endif

    return verified_memcpy_tofs((char *) statbuf, (char *) &tmp, sizeof(tmp));
}

int sys_stat(char *filename, struct stat *statbuf)
{
    struct inode *inode;
    int error = namei(filename, &inode, 0, 0);

    if (!error) {
	error = cp_stat(inode, statbuf);
	iput(inode);
    }

    return error;
}

int sys_lstat(char *filename, struct stat *statbuf)
{
    struct inode *inode;
    int error = lnamei(filename, &inode);

    if (!error) {
	error = cp_stat(inode, statbuf);
	iput(inode);
    }

    return error;
}

int sys_fstat(unsigned int fd, register struct stat *statbuf)
{
    struct file *f;
    int ret;

    ret = fd_check(fd, (char *) statbuf, sizeof(struct stat),
				FMODE_WRITE | FMODE_READ, &f);
    if (!ret)
	cp_stat(f->f_inode, statbuf);
    return ret;
}

int sys_readlink(char *path, char *buf, size_t bufsiz)
{
    struct inode *inode;
    register struct inode *pinode;
    register struct inode_operations *iop;
    int error = -EINVAL;

    if ((bufsiz > 0)
	&& !(error = verify_area(VERIFY_WRITE, buf, bufsiz))
	&& !(error = lnamei(path, &inode))
	) {
	pinode = inode;
	iop = pinode->i_op;
	error = (!iop || !iop->readlink)
	    ? (iput(pinode), -EINVAL)
	    : iop->readlink(pinode, buf, bufsiz);
    }

    return error;
}
