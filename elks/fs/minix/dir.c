/*
 *  linux/fs/minix/dir.c
 *
 *  Copyright (C) 1991, 1992 Linus Torvalds
 *
 *  minix directory handling functions
 */

#include <linuxmt/types.h>
#include <linuxmt/string.h>
#include <linuxmt/errno.h>
#include <linuxmt/fs.h>
#include <linuxmt/minix_fs.h>
#include <linuxmt/stat.h>
#include <linuxmt/config.h>

#include <arch/segment.h>

static size_t minix_dir_read(struct inode *inode, struct file *filp,
			     char *buf, int count)
{
    return -EISDIR;
}

static size_t minix_readdir(struct inode *inode, register struct file *filp,
			    char *dirent, filldir_t filldir);

/*@-type@*/

static struct file_operations minix_dir_operations = {
    NULL,			/* lseek - default */
    minix_dir_read,		/* read */
    NULL,			/* write - bad */
    minix_readdir,		/* readdir */
    NULL,			/* select - default */
    NULL,			/* ioctl - default */
    NULL,			/* no special open code */
    NULL,			/* no special release code */
#ifdef BLOAT_FS
    NULL			/* default fsync */
#endif
};

/*
 * directories can handle most operations...
 */
struct inode_operations minix_dir_inode_operations = {
    &minix_dir_operations,	/* default directory file-ops */
    minix_create,		/* create */
    minix_lookup,		/* lookup */
    minix_link,			/* link */
    minix_unlink,		/* unlink */
    minix_symlink,		/* symlink */
    minix_mkdir,		/* mkdir */
    minix_rmdir,		/* rmdir */
    minix_mknod,		/* mknod */
    NULL,			/* readlink */
    NULL,			/* follow_link */
#ifdef BLOAT_FS
    NULL,			/* bmap */
#endif
    minix_truncate		/* truncate */
#ifdef BLOAT_FS
	,
    NULL			/* permission */
#endif
};

/*@+type@*/

static size_t minix_readdir(struct inode *inode, register struct file *filp,
			    char *dirent, filldir_t filldir)
{
    register struct buffer_head *bh;
    struct minix_dir_entry *de;
    struct minix_sb_info *info;
    loff_t offset;

    if (!inode || !inode->i_sb || !S_ISDIR(inode->i_mode))
	return -EBADF;
    info = &inode->i_sb->u.minix_sb;
    if (filp->f_pos & (info->s_dirsize - 1))
	return -EBADF;
    while (filp->f_pos < (loff_t) inode->i_size) {
	offset = filp->f_pos & 1023;
	bh = minix_bread(inode,
			 (unsigned short int) (filp->f_pos >> BLOCK_SIZE_BITS),
			 0);
	if (!bh) {
	    filp->f_pos += 1024 - offset;
	    continue;
	} else {
	    map_buffer(bh);
	}
	do {
	    de = (struct minix_dir_entry *) (offset + bh->b_data);
	    if (de->inode) {
		size_t size = strnlen(de->name, info->s_namelen);
		if (filldir(dirent, de->name, size, filp->f_pos, de->inode)
		    < 0) {
		    unmap_brelse(bh);
		    return 0;
		}
	    }
	    offset += info->s_dirsize;
	    filp->f_pos += info->s_dirsize;
	} while (offset < 1024 && filp->f_pos < (loff_t) inode->i_size);
	unmap_brelse(bh);
    }
    return 0;
}
