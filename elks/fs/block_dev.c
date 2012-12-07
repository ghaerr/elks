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
    block_t block;
    kdev_t dev;
    unsigned int offset;
    size_t chars;
    int written;

    written = 0;
    dev = inode->i_rdev;

    while (count > 0) {

    /*
     *      Offset to block/offset
     */
    block = (block_t) (filp->f_pos >> 10);
	offset = ((unsigned int)filp->f_pos) & (BLOCK_SIZE - 1);

	/*
	 *      Bytes to do
	 */
	chars = BLOCK_SIZE - offset;
	if (chars > count)
	    chars = count;
	/*
	 *      Read the block in - use getblk on a write
	 *      of a whole block to avoid a read of the data.
	 */
	if(wr == BLOCK_WRITE && chars == BLOCK_SIZE) {
	    bh = getblk(dev, block);
	    map_buffer(bh);
	} else {
	    bh = bread(dev, block);
	if (!bh)
	    return written ? written : -EIO;
	}

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
		return -EIO;
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
    return ((wr == BLOCK_WRITE) && !written) ? -ENOSPC : written;
}

int block_read(struct inode *inode, register struct file *filp,
	       char *buf, size_t count)
{
    return blk_rw(inode, filp, buf, count, BLOCK_READ);
}

int block_write(struct inode *inode, register struct file *filp,
		char *buf, size_t count)
{
    return blk_rw(inode, filp, buf, count, BLOCK_WRITE);
}

#endif
