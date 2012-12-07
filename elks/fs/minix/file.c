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

#define	NBUF	2

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#include <linuxmt/fs.h>
#include <linuxmt/minix_fs.h>

static int minix_file_read(struct inode *inode, register struct file *filp,
			   char *buf, size_t icount);

static int minix_file_write(register struct inode *inode, struct file *filp,
			    char *buf, size_t count);

/*@-type@*/

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

static char inode_equal_NULL[] = "inode = NULL\n";
static char mode_equal_val[] = "mode = %07o\n";

static int minix_file_read(struct inode *inode, register struct file *filp,
			   char *buf, size_t icount)
{
    struct buffer_head *bh;
    loff_t offset, size, left;
    size_t chars, count = (icount % 65536);
    int read;
    block_t block, blocks;

    offset = filp->f_pos;
    size = (loff_t) inode->i_size;

    /*
     *      Amount we can do I/O over
     */

    left = (offset > size) ? 0 : size - offset;

    if (left > count)
	left = count;
    if (left <= 0) {
	debug("MFSREAD: EOF reached.\n");
	return 0;		/* EOF */
    }

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

    read = 0;
    /*
     *      Block, offset pair from the byte offset
     */
    block = (block_t) (offset >> BLOCK_SIZE_BITS);
    offset &= BLOCK_SIZE - 1;
    blocks = (block_t) ((offset + left + (BLOCK_SIZE - 1)) >> BLOCK_SIZE_BITS);

    while (blocks--) {
	debug1("MINREAD: Reading block #%d\n", block);
	if ((bh = minix_getblk(inode, block++, 0))) {
	    debug2("MINREAD: block %d = buffer %d\n", block - 1, bh->b_num);
	    if (!readbuf(bh)) {
		debug("MINREAD: readbuf failed\n");
		left = 0;
		break;
	    }
	}

	chars = (left < (BLOCK_SIZE - offset)) ? left : (BLOCK_SIZE - offset);

	filp->f_pos += chars;
	left -= chars;
	read += chars;
	if (bh) {
	    map_buffer(bh);
	    debug2("MINREAD: Copying data for block #%d, buffer #%d\n",
		 block - 1, bh->b_num);
	    memcpy_tofs(buf, offset + bh->b_data, (size_t) chars);
	    unmap_brelse(bh);
	    buf += chars;
	} else {
	    while (chars-- > 0)
		put_user_char((unsigned char)0, (void *)(buf++));
	}
	offset = 0;
    }
    if (!read)
	return -EIO;

#ifdef BLOAT_FS
    filp->f_reada = 1;
#endif
#ifdef FIXME
    if (!IS_RDONLY(inode))
	inode->i_atime = CURRENT_TIME;
#endif
    return read;
}

static int minix_file_write(register struct inode *inode,
			    struct file *filp, char *buf, size_t count)
{
    char *p;
    loff_t pos;
    size_t c, written;

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

    pos = (filp->f_flags & O_APPEND)
	? (loff_t) inode->i_size
	: filp->f_pos;

    written = 0;
    while (written < count) {
	register struct buffer_head *bh;

	bh = minix_getblk(inode, (unsigned short) (pos >> BLOCK_SIZE_BITS), 1);
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
    if (pos > (loff_t) inode->i_size)
	inode->i_size = (__u32) pos;
    inode->i_mtime = inode->i_ctime = CURRENT_TIME;
    filp->f_pos = pos;
    inode->i_dirt = 1;
    return (int) written;
}
