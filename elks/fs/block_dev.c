/*
 *  linux/fs/block_dev.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/config.h>

#ifdef CONFIG_BLK_DEV_CHAR

#include <linuxmt/errno.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/fs.h>
#include <linuxmt/locks.h>
#include <linuxmt/mm.h>

#include <arch/segment.h>
#include <arch/system.h>

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
		if (!written)
		    written = -EIO;
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
		if (!written)
		    written = -EIO;
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
