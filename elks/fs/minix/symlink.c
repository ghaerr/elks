/*
 *  linux/fs/minix/symlink.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  minix symlink handling code
 */

#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/sched.h>
#include <linuxmt/fs.h>
#include <linuxmt/mm.h>
#include <linuxmt/string.h>
#include <linuxmt/minix_fs.h>
#include <linuxmt/stat.h>

#include <arch/segment.h>

static int minix_follow_link(struct inode *dir,
			     register struct inode *inode,
			     int flag, mode_t mode, struct inode **res_inode)
{
    /*
     *      FIXME: #1 Stack use is too high
     *             #2 Needs to be current->link_count as this is blocking
     */

    int error;
    register struct buffer_head *bh;
    static int link_count = 0;
    seg_t ds, *pds;

    *res_inode = NULL;
    if (!dir) {
	dir = current->fs.root;
	dir->i_count++;
    }
    if (!inode) error = -ENOENT;
    else if (!S_ISLNK(inode->i_mode)) {
	*res_inode = inode;
	error = 0;
    } else if ( /* current-> */ link_count > 5) {
	iput(inode);
	error = -ELOOP;
    } else {
	bh = minix_bread(inode, 0, 0);
	iput(inode);
	if (!bh) error = -EIO;
	else {
	    /* current-> */ link_count++;
	    map_buffer(bh);
	    pds = &current->t_regs.ds;
	    ds = *pds;
	    *pds = kernel_ds;
	    error = open_namei(bh->b_data, flag, mode, res_inode, dir);
	    *pds = ds;
	    /* current-> */ link_count--;
	    unmap_brelse(bh);
	    return error;
	}
    }
    iput(dir);
    return error;
}

static int minix_readlink(register struct inode *inode,
			  char *buffer, size_t buflen)
{
    register struct buffer_head *bh;
    size_t len;

    if (!S_ISLNK(inode->i_mode)) len = -EINVAL;
    else {
	bh = minix_bread(inode, 0, 0);
	if (!bh) len = 0;
	else {
	    map_buffer(bh);
	    if ((len = strnlen(bh->b_data, 1023)) > buflen) len = buflen;
	    memcpy_tofs(buffer, bh->b_data, len);
	    unmap_brelse(bh);
	}
    }
    iput(inode);
    return len;
}

/*
 * symlinks can't do much...
 */

struct inode_operations minix_symlink_inode_operations = {
    NULL,			/* no file-operations */
    NULL,			/* create */
    NULL,			/* lookup */
    NULL,			/* link */
    NULL,			/* unlink */
    NULL,			/* symlink */
    NULL,			/* mkdir */
    NULL,			/* rmdir */
    NULL,			/* mknod */
    minix_readlink,		/* readlink */
    minix_follow_link,		/* follow_link */
    NULL,			/* getblk */
    NULL			/* truncate */
};
