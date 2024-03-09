/*
 *  linux-0.97/fs/msdos/file.c
 *
 *  Written 1992 by Werner Almesberger
 *
 *  MS-DOS regular file handling primitives
 *
 * Aug 2023 Greg Haerr - Don't use L1 cache for reading/writing file data.
 */

#include <arch/segment.h>
#include <arch/system.h>

#include <linuxmt/sched.h>
#include <linuxmt/fs.h>
#include <linuxmt/msdos_fs.h>
#include <linuxmt/errno.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/stat.h>
#include <linuxmt/mm.h>
#include <linuxmt/debug.h>

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

static size_t msdos_file_read(struct inode *inode,struct file *filp,char *buf,
    size_t count);
static size_t msdos_file_write(struct inode *inode,struct file *filp,char *buf,
    size_t count);


static struct file_operations msdos_file_operations = {
	NULL,			/* lseek - default */
	msdos_file_read,	/* read */
	msdos_file_write,	/* write */
	NULL,			/* readdir - bad */
	NULL,			/* select - default */
	NULL,			/* ioctl - default */
	NULL,			/* no special open is needed */
	NULL			/* release */
};

/* No getblk for MS-DOS FS' that don't align data at kByte boundaries. */

struct inode_operations msdos_file_inode_operations_no_bmap = {
	&msdos_file_operations,	/* default file operations */
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
	NULL,			/* getblk */
	msdos_truncate		/* truncate */
};


static size_t msdos_file_read(register struct inode *inode,register struct file *filp,
	char *buf,size_t count)
{
	char *start;
	size_t left, offset, secoff, size;
	sector_t sector;
	struct buffer_head *bh;

	//debug_fat("file_read cnt %u\n", count);
	if (!inode) {
		printk("FAT: read NULL inode\n");
		return -EINVAL;
	}
	if (!S_ISREG(inode->i_mode)) {
		printk("FAT: read bad mode %07o\n",inode->i_mode);
		return -EINVAL;
	}
	if (filp->f_pos >= inode->i_size || count <= 0) return 0;
	start = buf;
	while ((left = MIN(inode->i_size-filp->f_pos,count-(buf-start))) != 0) {
		if (!(sector = msdos_smap(inode,filp->f_pos >> SECTOR_BITS(inode))))
			break;
		offset = (int)filp->f_pos & (SECTOR_SIZE(inode)-1);
		if (!(bh = msdos_sread_nomap(inode->i_sb,sector, &secoff))) break;
		filp->f_pos += (size = MIN(SECTOR_SIZE(inode)-offset,left));
		xms_fmemcpyb(buf, current->t_regs.ds,
			buffer_data(bh) + offset + secoff, buffer_seg(bh), size);
		buf += size;
		brelse(bh);
	}
	if (start == buf) return -EIO;
	return buf-start;
}


static size_t msdos_file_write(register struct inode *inode,register struct file *filp,char *buf,
    size_t count)
{
	sector_t sector;
	size_t offset, secoff, size, written;
	int error;
	char *start;
	struct buffer_head *bh;

	debug_fat("file_write\n");
	if (!inode) {
		printk("FAT: write NULL inode\n");
		return -EINVAL;
	}
	if (!S_ISREG(inode->i_mode)) {
		printk("FAT: write bad mode %07o\n",inode->i_mode);
		return -EINVAL;
	}
/*
 * ok, append may not work when many processes are writing at the same time
 * but so what. That way leads to madness anyway.
 */
	if (filp->f_flags & O_APPEND) filp->f_pos = inode->i_size;
	if (count <= 0) return 0;
	error = 0;
	for (start = buf; count; count -= size) {
		while (!(sector = msdos_smap(inode,filp->f_pos >> SECTOR_BITS(inode))))
			if ((error = msdos_add_cluster(inode)) < 0) break;
		if (error) break;
		offset = (int)filp->f_pos & (SECTOR_SIZE(inode)-1);
		size = MIN(SECTOR_SIZE(inode)-offset,count);
		if (!(bh = msdos_sread_nomap(inode->i_sb,sector, &secoff))) {
			error = -EIO;
			break;
		}
		xms_fmemcpyb(buffer_data(bh) + offset + secoff, buffer_seg(bh),
			buf, current->t_regs.ds, written = size);
		buf += size;
		filp->f_pos += written;
		if (filp->f_pos > inode->i_size) {
			inode->i_size = filp->f_pos;
			inode->i_dirt = 1;
		}
		debug_fat("file block write %lu\n", buffer_blocknr(bh));
		mark_buffer_dirty(bh);
		brelse(bh);
	}
	inode->i_mtime = current_time();
	inode->u.msdos_i.i_attrs |= ATTR_ARCH;
	inode->i_dirt = 1;
	return start == buf ? error : buf-start;
}


void msdos_truncate(register struct inode *inode)
{
	cluster_t cluster;

	debug_fat("truncate inode %ld\n", inode->i_ino);
	cluster = (cluster_t)SECTOR_SIZE(inode)*MSDOS_SB(inode->i_sb)->cluster_size;
	(void)fat_free(inode,(inode->i_size + (cluster-1)) / cluster);
	inode->u.msdos_i.i_attrs |= ATTR_ARCH;
	inode->i_dirt = 1;
}
