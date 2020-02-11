/*
 *  linux-0.97/fs/msdos/file.c
 *
 *  Written 1992 by Werner Almesberger
 *
 *  MS-DOS regular file handling primitives
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

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

static int msdos_file_read(struct inode *inode,struct file *filp,char *buf,
    size_t count);
static int msdos_file_write(struct inode *inode,struct file *filp,char *buf,
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
#ifdef USE_GETBLK
	NULL,			/* getblk */
#endif
	msdos_truncate,		/* truncate */
#ifdef BLOAT_FS
	NULL
#endif
};


static int msdos_file_read(register struct inode *inode,register struct file *filp,
	char *buf,size_t count)
{
	char *start;
	size_t left,offset,size;
	long sector;
	struct buffer_head *bh;
	void *data;

//fsdebug("file_read cnt %u\n", count);
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
		if (!(sector = msdos_smap(inode,filp->f_pos >> SECTOR_BITS)))
			break;
		offset = (int)filp->f_pos & (SECTOR_SIZE-1);
		if (!(bh = msdos_sread(inode->i_dev,sector,&data))) break;
		filp->f_pos += (size = MIN(SECTOR_SIZE-offset,left));
		memcpy_tofs(buf,(char *)data+offset,size);
		buf += size;
		unmap_brelse(bh);
	}
	if (start == buf) return -EIO;
	return buf-start;
}


static int msdos_file_write(register struct inode *inode,register struct file *filp,char *buf,
    size_t count)
{
	long sector;
	int offset,size,written;
	int error;
	char *start;
	struct buffer_head *bh;
	void *data;

fsdebug("file_write\n");
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
		while (!(sector = msdos_smap(inode,filp->f_pos >> SECTOR_BITS)))
			if ((error = msdos_add_cluster(inode)) < 0) break;
		if (error) break;
		offset = (int)filp->f_pos & (SECTOR_SIZE-1);
		size = MIN(SECTOR_SIZE-offset,count);
		if (!(bh = msdos_sread(inode->i_dev,sector,&data))) {
			error = -EIO;
			break;
		}
		memcpy_fromfs((char *)data+((int)filp->f_pos & (SECTOR_SIZE-1)),
			    buf,written = size);
		buf += size;
		filp->f_pos += written;
		if (filp->f_pos > inode->i_size) {
			inode->i_size = filp->f_pos;
			inode->i_dirt = 1;
		}
fsdebug("file block write %d\n", bh->b_blocknr);
		bh->b_dirty = 1;
		unmap_brelse(bh);
	}
	inode->i_mtime = CURRENT_TIME;
	inode->u.msdos_i.i_attrs |= ATTR_ARCH;
	inode->i_dirt = 1;
	return start == buf ? error : buf-start;
}


void msdos_truncate(register struct inode *inode)
{
	int cluster;

fsdebug("truncate\n");
	cluster = SECTOR_SIZE*MSDOS_SB(inode->i_sb)->cluster_size;
	(void)fat_free(inode,(inode->i_size + (cluster-1)) / cluster);
	inode->u.msdos_i.i_attrs |= ATTR_ARCH;
	inode->i_dirt = 1;
}
