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

static size_t minix_dir_read(struct inode *inode,struct file *filp,char *buf,size_t count)
{
    return -EISDIR;
}

static int minix_readdir(struct inode *inode,
			 register struct file *filp,
			 char *dirent, filldir_t filldir);

static struct file_operations minix_dir_operations = {
    NULL,			/* lseek - default */
    minix_dir_read,		/* read */
    NULL,			/* write - bad */
    minix_readdir,		/* readdir */
    NULL,			/* select - default */
    NULL,			/* ioctl - default */
    NULL,			/* no special open code */
    NULL			/* no special release code */
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
    NULL,			/* getblk */
    minix_truncate		/* truncate */
};

static int minix_readdir(struct inode *inode,
			 register struct file *filp,
			 char *dirent, filldir_t filldir)
{
    register struct buffer_head *bh;
    struct minix_dir_entry *de;
    struct minix_sb_info *info;
    unsigned short offset;

    if (!inode || !inode->i_sb || !S_ISDIR(inode->i_mode)) return -EBADF;
    info = &inode->i_sb->u.minix_sb;
    if (((unsigned short)filp->f_pos) & (info->s_dirsize - 1)) return -EBADF;
    while (filp->f_pos < (loff_t) inode->i_size) {
	offset = (unsigned short)filp->f_pos & (BLOCK_SIZE - 1);
	bh = minix_bread(inode, (block_t) ((filp->f_pos) >> BLOCK_SIZE_BITS), 0);
	if (!bh) {
	    filp->f_pos += (BLOCK_SIZE - offset);
	    continue;
	} else map_buffer(bh);
	do {
	    de = (struct minix_dir_entry *) (offset + bh->b_data);
	    if (de->inode) {
			filldir(dirent, de->name, strnlen(de->name, info->s_namelen),
				filp->f_pos, de->inode);
			filp->f_pos += info->s_dirsize;
		    unmap_brelse(bh);
		    return 0;
	    }
	    offset += info->s_dirsize;
	    filp->f_pos += info->s_dirsize;
	} while (offset < BLOCK_SIZE && filp->f_pos < (loff_t)inode->i_size);
	unmap_brelse(bh);
    }
    return 0;
}
