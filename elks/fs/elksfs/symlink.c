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
#include <linuxmt/elksfs_fs.h>
#include <linuxmt/stat.h>
#include <linuxmt/mm.h>

#include <arch/segment.h>

static int elksfs_readlink();
static int elksfs_follow_link();

/*
 * symlinks can't do much...
 */
struct inode_operations elksfs_symlink_inode_operations = {
    NULL,			/* no file-operations */
    NULL,			/* create */
    NULL,			/* lookup */
    NULL,			/* link */
    NULL,			/* unlink */
    NULL,			/* symlink */
    NULL,			/* mkdir */
    NULL,			/* rmdir */
    NULL,			/* mknod */
    elksfs_readlink,		/* readlink */
    elksfs_follow_link,		/* follow_link */
#ifdef BLOAT_FS
    NULL,			/* bmap */
#endif
    NULL			/* truncate */
#ifdef BLOAT_FS
	,
    NULL			/* permission */
#endif
};

static int elksfs_follow_link(register struct inode *dir,
			      register struct inode *inode,
			      int flag, int mode, struct inode **res_inode)
{
    int error;
    struct buffer_head *bh;

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
    if (link_count > 7) {
	iput(inode);
	iput(dir);
	return -ELOOP;
    }
    if (!(bh = elksfs_bread(inode, 0, 0))) {
	iput(inode);
	iput(dir);
	return -EIO;
    }
    iput(inode);
    link_count++;
    map_buffer(bh);
    error = open_namei(bh->b_data, flag, mode, res_inode, dir);
    link_count--;
    unmap_brelse(bh);
    return error;
}

static int elksfs_readlink(register struct inode *inode,
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
    bh = elksfs_bread(inode, 0, 0);
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
