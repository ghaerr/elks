/*
 *  linux/fs/minix/file.c
 *
 *  Copyright (C) 1991, 1992 Linus Torvalds
 *
 *  minix regular file handling primitives
 */

#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/elksfs_fs.h>
#include <linuxmt/kernel.h>
#include <linuxmt/errno.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/stat.h>
#include <linuxmt/mm.h>
#include <linuxmt/debug.h>

#include <arch/segment.h>
#include <arch/system.h>

#define	NBUF	2

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#include <linuxmt/fs.h>
#include <linuxmt/elksfs_fs.h>

/*
 *	FIXME: Readahead
 */

static int elksfs_file_read(struct inode *inode, register struct file *filp,
			    char *buf, size_t icount)
{
    /*  We have to make count long since comparing ints to longs
     *  does not work with bcc!
     */

    unsigned long int count = (unsigned long int) (icount % 65536);
    struct buffer_head *bh;
    block_t block, blocks;
    loff_t offset;
    size_t chars, left, size;
    int read;

    offset = filp->f_pos;
    size = inode->i_size;

    /*
     *      Amount we can do I/O over
     */

    if (offset > (loff_t) size)
	left = 0;
    else
	left = size - offset;
    if (left > count)
	left = count;
    if (left <= 0) {
	debug("MFSREAD: EOF reached.\n");
	return 0;		/* EOF */
    }
    if (!inode) {
	printk("elksfs_file_read: inode = NULL\n");
	return -EINVAL;
    }
    if (!S_ISREG(inode->i_mode)) {
	printk("elksfs_file_read: mode = %07o\n", inode->i_mode);
	return -EINVAL;
    }

    read = 0;
    /*
     *      Block, offset pair from the byte offset
     */
    block = (block_t) (offset >> BLOCK_SIZE_BITS);
    offset &= BLOCK_SIZE - 1;
    blocks = (offset + left + (BLOCK_SIZE - 1)) >> BLOCK_SIZE_BITS;

    while (blocks) {
	--blocks;
	debug1("MINREAD: Reading block #%ld\n", block);
	if ((bh = elksfs_getblk(inode, block++, 0))) {
	    debug2("MINREAD: block %ld = buffer %d\n", block - 1, bh->b_num);
	    if (!readbuf(bh)) {
		debug("MINREAD: readbuf failed\n");
		left = 0;
		break;
	    }
	}

	if (left < (BLOCK_SIZE - offset))
	    chars = left;
	else
	    chars = BLOCK_SIZE - offset;

	filp->f_pos += chars;
	left -= chars;
	read += chars;
	if (bh) {
	    map_buffer(bh);
	    debug2("MINREAD: Copying data for block #%ld, buffer #%d\n",
		 block - 1, bh->b_num);
	    memcpy_tofs(buf, offset + bh->b_data, chars);
	    unmap_brelse(bh);
	    buf += chars;
	} else {
	    char zero = 0;
	    while (chars-- > 0)
		memcpy_tofs(buf++, &zero, 1);
	}
	offset = 0;
    }
    while (left > 0);
    if (!read)
	return -EIO;

#ifdef BLOAT_FS
    filp->f_reada = 1;
#endif

#if 0					/* FIXME */
    if (!IS_RDONLY(inode))
	inode->i_atime = CURRENT_TIME;
#endif

    return read;
}

static int elksfs_file_write(register struct inode *inode,
			     struct file *filp, char *buf, size_t count)
{
#ifdef CONFIG_FS_RO
    return -EINVAL;
#else
    off_t pos;
    size_t written, c;
    register struct buffer_head *bh;
    char *p;

    if (!inode) {
	printk("elksfs_file_write: inode = NULL\n");
	return -EINVAL;
    }
    if (!S_ISREG(inode->i_mode)) {
	printk("elksfs_file_write: mode = %07o\n", inode->i_mode);
	return -EINVAL;
    }
    if (filp->f_flags & O_APPEND)
	pos = (off_t) inode->i_size;
    else
	pos = filp->f_pos;
    written = 0;
    while (written < count) {
	bh = elksfs_getblk(inode,
			   (unsigned long int) (pos >> BLOCK_SIZE_BITS), 1);
	if (!bh) {
	    if (!written)
		written = -ENOSPC;
	    break;
	}
	c = BLOCK_SIZE - (pos % BLOCK_SIZE);
	if (c > count - written)
	    c = count - written;
	if (c != BLOCK_SIZE && !buffer_uptodate(bh)) {
	    if (!readbuf(bh)) {
		if (!written)
		    written = -EIO;
		break;
	    }
	}
	map_buffer(bh);
	p = (pos % BLOCK_SIZE) + bh->b_data;
	memcpy_fromfs(p, buf, c);
	mark_buffer_uptodate(bh, 1);
	mark_buffer_dirty(bh, 1);
	unmap_brelse(bh);
	pos += c;
	written += c;
	buf += c;
    }
    if (pos > (off_t) inode->i_size)
	inode->i_size = (__u32) pos;
    inode->i_mtime = inode->i_ctime = CURRENT_TIME;
    filp->f_pos = pos;
    inode->i_dirt = 1;
    return (int) written;
#endif
}

/*@-type@*/

/*
 * We have mostly NULL's here: the current defaults are ok for
 * the minix filesystem.
 */
static struct file_operations elksfs_file_operations = {
    NULL,			/* lseek - default */
    elksfs_file_read,		/* read */
#ifdef CONFIG_FS_RO
    NULL,			/* write */
#else
    elksfs_file_write,		/* write */
#endif
    NULL,			/* readdir - bad */
    NULL,			/* select - default */
    NULL,			/* ioctl - default */
    NULL,			/* no special open is needed */
    NULL			/* release */
#ifdef BLOAT_FS
	,
    NULL			/* fsync : minix_file_fsync */
#endif
};

struct inode_operations elksfs_file_inode_operations = {
    &elksfs_file_operations,	/* default file operations */
    NULL,			/* create */
    NULL,			/* lookup */
    NULL,			/* link */
    NULL,			/* unlink */
    NULL,			/* symlink */
    NULL,			/* mkdir */
    NULL,			/* rmdir */
    NULL,			/* mknod */
    NULL,			/* readlink */
    NULL			/* follow_link */
#ifdef BLOAT_FS
	,
    minix_bmap  		/* bmap */
#endif
#ifdef CONFIG_FS_RO
	,
    NULL			/* truncate */
#else
	,
    elksfs_truncate		/* truncate */
#endif
#ifdef BLOAT_FS
	,
    NULL			/* permission */
#endif
};

/*@+type@*/
