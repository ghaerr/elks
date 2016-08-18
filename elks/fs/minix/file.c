/*
 *  linux/fs/minix/file.c
 *
 *  Copyright (C) 1991, 1992 Linus Torvalds
 *
 *  minix regular file handling primitives
 */

#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/minix_fs.h>
#include <linuxmt/kernel.h>
#include <linuxmt/errno.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/stat.h>
#include <linuxmt/mm.h>
#include <linuxmt/debug.h>

#include <arch/segment.h>
#include <arch/system.h>

#include <linuxmt/fs.h>
#include <linuxmt/minix_fs.h>

static size_t minix_file_read(struct inode *inode, register struct file *filp,
			    char *buf, size_t count);

static size_t minix_file_write(struct inode *inode,
			    register struct file *filp, char *buf, size_t count);

/*
 * We have mostly NULL's here: the current defaults are ok for
 * the minix filesystem.
 */
static struct file_operations minix_file_operations = {
    NULL,			/* lseek - default */
    minix_file_read,		/* read */
    minix_file_write,		/* write */
    NULL,			/* readdir - bad */
    NULL,			/* select - default */
    NULL,			/* ioctl - default */
    NULL,			/* no special open is needed */
    NULL,			/* release */
#ifdef BLOAT_FS
    NULL			/* fsync : minix_file_fsync */
#endif
};

struct inode_operations minix_file_inode_operations = {
    &minix_file_operations,	/* default file operations */
    NULL,			/* create */
    NULL,			/* lookup */
    NULL,			/* link */
    NULL,			/* unlink */
    NULL,			/* symlink */
    NULL,			/* mkdir */
    NULL,			/* rmdir */
    NULL,			/* mknod */
    NULL,			/* readlink */
    NULL,			/* follow_link */
#ifdef BLOAT_FS
    NULL /*minix_bmap */ ,	/* bmap */
#endif
    minix_truncate,		/* truncate */
#ifdef BLOAT_FS
    NULL			/* permission */
#endif
};

/*@+type@*/

/*
 *	FIXME: Readahead
 */
#ifdef DEBUG
static char inode_equal_NULL[] = "inode = NULL\n";
static char mode_equal_val[] = "mode = %07o\n";
#endif

static size_t minix_file_read(struct inode *inode, register struct file *filp,
			    char *buf, size_t count)
{
    register struct buffer_head *bh;
    loff_t pos;
    size_t chars, offset;
    int read = 0;

#ifdef DEBUG
    {
	register char *s;
	if (!inode) {
	    s = inode_equal_NULL;
	    goto OUTPUT;
	} else if (!S_ISREG(inode->i_mode)) {
	    s = mode_equal_val; /* i_mode */
	OUTPUT:
	    printk("minix_file_read: ");
	    printk(s, inode->i_mode);
	    return -EINVAL;
	}
    }
#endif

    /*
     *      Amount we can do I/O over
     */
    pos = ((loff_t)inode->i_size) - filp->f_pos;
    if (pos <= 0) {
	debug("MFSREAD: EOF reached.\n");
	goto mfread;		/* EOF */
    }
    if ((loff_t)count > pos)
	count = (size_t)pos;

    while (count > 0) {
    /*
     *      Offset to block/offset
     */
	offset = ((size_t)filp->f_pos) & (BLOCK_SIZE - 1);
	chars = BLOCK_SIZE - offset;
	if (chars > count)
	    chars = count;
	/*
	 *      Read the block in - use getblk on a write
	 *      of a whole block to avoid a read of the data.
	 */
	bh = minix_getblk(inode, (block_t)(filp->f_pos >> BLOCK_SIZE_BITS), 0);
	if (bh) {
	    if (!readbuf(bh)) {
		debug("MINREAD: readbuf failed\n");
		if (!read)
		    read = -EIO;
		break;
	    }
	    map_buffer(bh);
	    memcpy_tofs(buf, bh->b_data + (size_t)offset, chars);
	    unmap_brelse(bh);
	} else {
	    fmemset(buf, current->t_regs.ds, 0, chars);
	}
	buf += chars;
	filp->f_pos += chars;
	read += chars;
	count -= chars;
    }

#ifdef BLOAT_FS
    filp->f_reada = 1;
#endif
#ifdef FIXME
    if (!IS_RDONLY(inode))
	inode->i_atime = CURRENT_TIME;
#endif
  mfread:
    return read;
}

static size_t minix_file_write(struct inode *inode,
			    register struct file *filp, char *buf, size_t count)
{
    size_t chars, offset;
    int written = 0;

#ifdef DEBUG
    {
	register char *s;
	if (!inode) {
	    s = inode_equal_NULL;
	    goto OUTPUT;
	}
	if (!S_ISREG(inode->i_mode)) {
	    s = mode_equal_val; /* inode->i_mode */
	OUTPUT:
	    printk("minix_file_write: ");
	    printk(s, inode->i_mode);
	    return -EINVAL;
	}
    }
#endif

    if (filp->f_flags & O_APPEND)
	filp->f_pos = (loff_t)inode->i_size;

    while (count > 0) {
	register struct buffer_head *bh;
    /*
     *      Offset to block/offset
     */
	offset = ((size_t)filp->f_pos) & (BLOCK_SIZE - 1);
	chars = BLOCK_SIZE - offset;
	if (chars > count)
	    chars = count;
	/*
	 *      Read the block in - use getblk on a write
	 *      of a whole block to avoid a read of the data.
	 */
	bh = minix_getblk(inode, (block_t) (filp->f_pos >> BLOCK_SIZE_BITS), 1);
	if (!bh) {
	    if (!written)
		written = -ENOSPC;
	    break;
	}
	if (chars != BLOCK_SIZE) {
	    if (!readbuf(bh)) {
		if (!written)
		    written = -EIO;
		break;
	    }
	}
	map_buffer(bh);
	memcpy_fromfs((bh->b_data + offset), buf, chars);
	mark_buffer_uptodate(bh, 1);
	mark_buffer_dirty(bh, 1);
	unmap_brelse(bh);
	buf += chars;
	filp->f_pos += chars;
	written += chars;
	count -= chars;
    }
    {
	register struct inode *pinode = inode;
	if ((loff_t)pinode->i_size < filp->f_pos)
	    pinode->i_size = (__u32) filp->f_pos;
	pinode->i_mtime = pinode->i_ctime = CURRENT_TIME;
	pinode->i_dirt = 1;
    }
    return written;
}
