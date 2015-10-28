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
#include <linuxmt/minix_fs.h>
#include <linuxmt/stat.h>

#include <arch/segment.h>

static int minix_follow_link(register struct inode *dir,
			     register struct inode *inode,
			     int flag, int mode, struct inode **res_inode)
{

    /*
     *      FIXME: #1 Stack use is too high
     *             #2 Needs to be current->link_count as this is blocking
     */

    int error;
    struct buffer_head *bh;
    static int link_count = 0;
    __u16 ds, *pds;

    *res_inode = NULL;
    if (!dir) {
	dir = current->fs.root;
	dir->i_count++;
    }
    if (!inode) {
	iput(dir);
	return -ENOENT;
    }
    if (!S_ISLNK(inode->i_mode)) {
	iput(dir);
	*res_inode = inode;
	return 0;
    }
    if ( /* current-> */ link_count > 5) {
	iput(inode);
	iput(dir);
	return -ELOOP;
    }
    if (!(bh = minix_bread(inode, 0, 0))) {
	iput(inode);
	iput(dir);
	return -EIO;
    }
    iput(inode);
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

static int minix_readlink(register struct inode *inode,
			  char *buffer, int buflen)
{
    register struct buffer_head *bh;
    size_t len;

    if (!S_ISLNK(inode->i_mode)) {
	iput(inode);
	return -EINVAL;
    }
    bh = minix_bread(inode, 0, 0);
    iput(inode);

    if (!bh)
	return 0;
    map_buffer(bh);

    if((len = strlen(bh->b_data) + 1) > buflen)
	len = buflen;
    if (len > 1023)
	len = 1023;
    memcpy_tofs(buffer, bh->b_data, len);
    unmap_brelse(bh);
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
#ifdef BLOAT_FS
    NULL,			/* bmap */
#endif
    NULL,			/* truncate */
#ifdef BLOAT_FS
    NULL			/* permission */
#endif
};
