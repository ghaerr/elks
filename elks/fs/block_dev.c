/*
 *  linux/fs/block_dev.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/errno.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/mm.h>

#include <arch/segment.h>
#include <arch/system.h>

int blk_rw(inode,filp,buf,count,wr)
struct inode * inode;
register struct file * filp;
char * buf;
int count;
int wr;
{
	int write_error;
	block_t block;
	off_t offset;
	int chars;
	int written = 0;
	kdev_t dev;
	struct buffer_head * bh;
	char * p;

	write_error = 0;
	dev = inode->i_rdev;

	/*
	 *	Offset to block/offset
	 */
	 
	block = filp->f_pos >> 10;
	offset = filp->f_pos & (BLOCK_SIZE-1);

	while (count>0) {
		/*
		 *	Bytes to do
		 */
		chars = BLOCK_SIZE - offset;
		if (chars > count)
			chars=count;
		/*
		 *	Read the block in - use getblk on a write
		 *	of a whole block to avoid a read of the data.
		 */
		if (wr == BLOCK_WRITE && chars == BLOCK_SIZE) {
			bh = getblk(dev,block);
		} else {
			bh = bread(dev,block);
		}
		block++;
		if (!bh) {
			return written?written:-EIO;
		}
/*		p = offset + bh->b_data; */
/*		offset = 0; */
		filp->f_pos += chars;
		written += chars;
		count -= chars;
		/*
		 *	What are we doing ?
		 */
		map_buffer(bh);
		p = offset + bh->b_data;
		if (wr == BLOCK_WRITE) {
			/*
			 *	Alter buffer, mark dirty
			 */
			memcpy_fromfs(p,buf,chars);
			bh->b_uptodate = 1;
			bh->b_dirty = 1;
		} else {
			/*
			 *	Empty buffer data. Buffer unchanged
			 */
			memcpy_tofs(buf,p,chars);
		}
		/*
		 *	Move on and release buffer
		 */
/*		p += chars; */
		offset = 0;
		buf += chars;
		if (wr == BLOCK_WRITE) {
			/*
			 *	Writing: queue physical I/O
			 */
			ll_rw_blk(WRITE, bh);
			wait_on_buffer(bh);
			if (!bh->b_uptodate) {
				write_error=1;
			}
		}
		unmap_brelse(bh);
		if(write_error)
			break;
	}
	if (write_error)
		return -EIO;
	if ((wr == BLOCK_WRITE) && !written)
		return ENOSPC;
	return written;
}

int block_read(inode,filp,buf,count)
register struct inode * inode;
register struct file * filp;
register char * buf;
int count;
{
	blk_rw(inode,filp,buf,count,BLOCK_READ);
}

int block_write(inode,filp,buf,count)
register struct inode * inode;
register struct file * filp;
register char * buf;
int count;
{
	blk_rw(inode,filp,buf,count,BLOCK_WRITE);
}

