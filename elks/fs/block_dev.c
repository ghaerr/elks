/*
 *  linux/fs/block_dev.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/config.h>

#include <linuxmt/errno.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/fs.h>
#include <linuxmt/mm.h>
#include <linuxmt/debug.h>
#ifdef DEBUG
#	include <linuxmt/stat.h>
#endif

#include <arch/segment.h>
#include <arch/system.h>

#ifdef USE_GETBLK
#if defined(CONFIG_MINIX_FS) || defined(CONFIG_BLK_DEV_CHAR)

#ifdef DEBUG
static char inode_equal_NULL[] = "inode = NULL\n";
static char mode_equal_val[] = "mode = %07o\n";
#endif

size_t block_read(struct inode *inode, register struct file *filp,
	       char *buf, size_t count)
{
    loff_t pos;
    size_t chars;
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
	    printk("generic_block_read: ");
	    printk(s, inode->i_mode);
	    return -EINVAL;
	}
    }
#endif

    /* Amount we can do I/O over */
    pos = ((loff_t)inode->i_size) - filp->f_pos;
    if (pos <= 0) {
	debug("GENREAD: EOF reached.\n");
	goto mfread;		/* EOF */
    }
    if ((loff_t)count > pos) count = (size_t)pos;

    while (count > 0) {
	register struct buffer_head *bh;

	/*
	 *      Read the block in
	 */
	chars = (filp->f_pos >> BLOCK_SIZE_BITS);
	if (inode->i_op->getblk)
	    bh = inode->i_op->getblk(inode, (block_t)chars, 0);
	else
	    bh = getblk(inode->i_rdev, (block_t)chars);
	/* Offset to block/offset */
	chars = BLOCK_SIZE - (((size_t)(filp->f_pos)) & (BLOCK_SIZE - 1));
	if (chars > count) chars = count;
	if (bh) {
	    if (!readbuf(bh)) {
		debug("GENREAD: readbuf failed\n");
		if (!read) read = -EIO;
		break;
	    }
	    map_buffer(bh);
	    memcpy_tofs(buf, bh->b_data + (((size_t)(filp->f_pos)) & (BLOCK_SIZE - 1)),
			chars);
	    unmap_brelse(bh);
	} else fmemsetb((word_t) buf, current->t_regs.ds, 0, (word_t) chars);
	buf += chars;
	filp->f_pos += chars;
	read += chars;
	count -= chars;
    }

#ifdef BLOAT_FS
    filp->f_reada = 1;
#endif
#ifdef FIXME
    if (!IS_RDONLY(inode)) inode->i_atime = CURRENT_TIME;
#endif
  mfread:
    return read;
}

size_t block_write(struct inode *inode, register struct file *filp,
		char *buf, size_t count)
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
	    printk("generic_block_write: ");
	    printk(s, inode->i_mode);
	    return -EINVAL;
	}
    }
#endif

    if (filp->f_flags & O_APPEND) filp->f_pos = (loff_t)inode->i_size;

    while (count > 0) {
	register struct buffer_head *bh;

	chars = (filp->f_pos >> BLOCK_SIZE_BITS);
	if (inode->i_op->getblk)
	    bh = inode->i_op->getblk(inode, (block_t)chars, 1);
	else
	    bh = getblk(inode->i_rdev, (block_t)chars);
	if (!bh) {
	    if (!written) written = -ENOSPC;
	    break;
	}
	/* Offset to block/offset */
	offset = ((size_t)filp->f_pos) & (BLOCK_SIZE - 1);
	chars = BLOCK_SIZE - offset;
	if (chars > count) chars = count;
	/*
	 *      Read the block in, unless we
	 *      are writing a whole block.
	 */
	if (chars != BLOCK_SIZE) {
	    if (!readbuf(bh)) {
		if (!written) written = -EIO;
		break;
	    }
	}
	/*
	 *      Alter buffer, mark dirty
	 */
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
#endif
#else
#ifdef CONFIG_BLK_DEV_CHAR

static int blk_rw(struct inode *inode, register struct file *filp,
		  char *buf, size_t count, int wr)
{
    register struct buffer_head *bh;
    size_t chars, offset;
    int written = 0;

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
	bh = getblk(inode->i_rdev, (block_t)(filp->f_pos >> BLOCK_SIZE_BITS));
	if ((wr == BLOCK_READ) || (chars != BLOCK_SIZE)) {
	    if (!readbuf(bh)) {
		if (!written) written = -EIO;
		break;
	    }
	}

	map_buffer(bh);
	if (wr == BLOCK_WRITE) {
	    /*
	     *      Alter buffer, mark dirty
	     */
	    memcpy_fromfs(bh->b_data + offset, buf, chars);
	    bh->b_uptodate = bh->b_dirty = 1;
	    /*
	     *      Writing: queue physical I/O
	     */
	    ll_rw_blk(WRITE, bh);
	    wait_on_buffer(bh);
	    if (!bh->b_uptodate) { /* Write error. */
		unmap_brelse(bh);
		if (!written) written = -EIO;
		break;
	    }
	} else {
	    /*
	     *      Empty buffer data. Buffer unchanged
	     */
	    memcpy_tofs(buf, bh->b_data + offset, chars);
	}
	/*
	 *      Move on and release buffer
	 */

	unmap_brelse(bh);

	buf += chars;
	filp->f_pos += chars;
	written += chars;
	count -= chars;
    }
    return written;
}

size_t block_read(struct inode *inode, struct file *filp,
	       char *buf, size_t count)
{
    return blk_rw(inode, filp, buf, count, BLOCK_READ);
}

size_t block_write(struct inode *inode, struct file *filp,
		char *buf, size_t count)
{
    return blk_rw(inode, filp, buf, count, BLOCK_WRITE);
}

#endif
#endif
