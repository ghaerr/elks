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
    error = open_namei(bh->b_data, flag, mode, res_inode, dir);
    /* current-> */ link_count--;
    unmap_brelse(bh);
    return error;
}

static int minix_readlink(register struct inode *inode,
			  char *buffer, int buflen)
{
    register struct buffer_head *bh;
    int i;
    char c;

    if (!S_ISLNK(inode->i_mode)) {
	iput(inode);
	return -EINVAL;
    }
    if (buflen > 1023)
	buflen = 1023;
    bh = minix_bread(inode, 0, 0);
    iput(inode);
    if (!bh)
	return 0;
    map_buffer(bh);
    i = 0;
    while (i < buflen && (c = bh->b_data[i])) {
	i++;
	memcpy_tofs(buffer++, &c, 1);
	/* put_fs_byte(c,buffer++); */
    }
    unmap_brelse(bh);
    return i;
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
