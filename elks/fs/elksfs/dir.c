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
#include <linuxmt/elksfs_fs.h>
#include <linuxmt/stat.h>
#include <linuxmt/config.h>

#include <arch/segment.h>

static int elksfs_dir_read(inode,filp,buf,count)
struct inode * inode;
struct file * filp;
char * buf;
int count;
{
	return -EISDIR;
}

static int elksfs_readdir();

static struct file_operations elksfs_dir_operations = {
	NULL,			/* lseek - default */
	elksfs_dir_read,		/* read */
	NULL,			/* write - bad */
	elksfs_readdir,		/* readdir */
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
struct inode_operations elksfs_dir_inode_operations = {
	&elksfs_dir_operations,	/* default directory file-ops */
#ifdef CONFIG_FS_RO
	NULL,			/* create */
	elksfs_lookup,		/* lookup */
	NULL,			/* link */
	NULL,			/* unlink */
	NULL,			/* symlink */
	NULL,			/* mkdir */
	NULL,			/* rmdir */
	NULL,			/* mknod */
	NULL,			/* readlink */
	NULL,			/* follow_link */
#ifdef BLOAT_FS
	NULL,			/* bmap */
#endif
	NULL,			/* truncate */
#else /* CONFIG_FS_RO */
	elksfs_create,		/* create */
	elksfs_lookup,		/* lookup */
	elksfs_link,		/* link */
	elksfs_unlink,		/* unlink */
	elksfs_symlink,		/* symlink */
	elksfs_mkdir,		/* mkdir */
	elksfs_rmdir,		/* rmdir */
	elksfs_mknod,		/* mknod */
	NULL,			/* readlink */
	NULL,			/* follow_link */
#ifdef BLOAT_FS
	NULL,			/* bmap */
#endif
	elksfs_truncate,		/* truncate */
#endif /* CONFIG_FS_RO */
#ifdef BLOAT_FS
	NULL			/* permission */
#endif
};

static int elksfs_readdir(inode,filp, dirent,filldir)
struct inode * inode;
register struct file * filp;
char * dirent;
filldir_t filldir;
{
	unsigned int offset;
	struct buffer_head * bh;
	struct elksfs_dir_entry * de;
	register struct elksfs_sb_info * info;

	if (!inode || !inode->i_sb || !S_ISDIR(inode->i_mode))
		return -EBADF;
	info = &inode->i_sb->u.elksfs_sb;
	if (filp->f_pos & (info->s_dirsize - 1))
		return -EBADF;
	while (filp->f_pos < inode->i_size) {
		offset = filp->f_pos & 1023;
		bh = elksfs_bread(inode,(filp->f_pos)>>BLOCK_SIZE_BITS,0);
		if (!bh) {
			filp->f_pos += 1024-offset;
			continue;
		} else {
			map_buffer(bh);
		}
		do {
			de = (struct elksfs_dir_entry *) (offset + bh->b_data);
			if (de->inode) {
				int size = strnlen(de->name, info->s_namelen);
				if (filldir(dirent, de->name, size, filp->f_pos, de->inode) < 0) {
					unmap_brelse(bh);
					return 0;
				}
			}
			offset += info->s_dirsize;
			filp->f_pos += info->s_dirsize;
		} while (offset < 1024 && filp->f_pos < inode->i_size);
		unmap_brelse(bh);
	}
	return 0;
}
