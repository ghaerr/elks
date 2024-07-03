/* ROMFS - A tiny read-only filesystem in memory */

#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/fs.h>
#include <linuxmt/sched.h>
#include <linuxmt/stat.h>
#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/string.h>
#include <linuxmt/debug.h>
#include <arch/segment.h>


/* File operations */

static size_t romfs_read (struct inode * i, struct file * f,
	char * buf, size_t len)
{
	size_t count;

	while (1) {
		/* Some programs do not check the size and read until EOF (cat) */
		/* Limit the length in such case */

		if (f->f_pos >= i->i_size) {
			count = 0;
			break;
		}

		if (f->f_pos + len > i->i_size) {
			len = i->i_size - f->f_pos;
		}

		/* ELKS trick: the destination buffer is in the current task data segment */
		fmemcpyb (buf, current->t_regs.ds, (char *)(int) f->f_pos, i->u.romfs.seg, len);

		f->f_pos += len;
		count = len;
		break;
	}

	return count;
}


/* Directory operations */

static int romfs_readdir (struct inode * i, struct file * f,
	void * dirent, filldir_t filldir)
{
	int res;

	size_t len;
	word_t pos;
	seg_t iseg;
	char name [ROMFS_NAME_MAX]; /* FIXME 255 is too large for stack buffer*/

	while (1) {
		iseg = i->u.romfs.seg;
		pos = f->f_pos;

		/* Only one entry is asked by the kernel */

		if (pos >= i->i_size) {
			f->f_pos = -1;
			res = 0;
			break;
		}

		len = peekb (pos + 2, iseg);
		if (!len || len > ROMFS_NAME_MAX) {
			res = 0;
			break;
		}

		fmemcpyb (name, kernel_ds, (char *)pos + 3, iseg, len);

		res = filldir (dirent, name, len, pos, (ino_t)peekw(pos, iseg));
		debug("readdir %T, %ld\n", iseg, pos + 3, (ino_t)peekw(pos, iseg));
		if (res < 0) break;

		/* inode index + name length + name string */
		f->f_pos = pos + 3 + len;
		break;
	}

	return res;
}


static int romfs_lookup (struct inode * dir, const char * name, size_t len1,
	struct inode ** result)
{
	int res;
	struct inode * i;

	word_t ino;
	word_t offset;
	size_t len2;
	seg_t seg_i;

	while (1) {
		seg_i = dir->u.romfs.seg;

		ino = 0;
		offset = 0;

		while (offset < dir->i_size) {
			debug("romfs lookup %t, %T\n", name, seg_i, offset+3);
			len2 = peekb (offset + 2, seg_i);
			/* ELKS trick: the name is in the current task data segment */
			/* TODO: remove that trick with explicit segment in call */
			if (len2 == len1) {
				if (!fmemcmpb ((char *)offset + 3, seg_i, (void *)name, current->t_regs.ds, len2)) {
					ino = peekw (offset, seg_i);
					break;
				}
			}

			/* inode index + name length + name string */
			offset += 3 + len2;
		}

		if (!ino) {
			res = -ENOENT;
			break;
		}

		i = iget (dir->i_sb, (ino_t) ino);
		if (!i) {
			res = -EACCES;
			break;
		}

		*result = i;
		res = 0;
		break;
	}

	iput (dir);  /* really needed ? */
	return res;
}


static int romfs_readlink (struct inode * inode, char * buf, size_t len)
{
	size_t count;

	count = inode->i_size;
	if (count > len) count = len;
	/* ELKS trick: the destination buffer is in the current task data segment */
	fmemcpyb (buf, current->t_regs.ds, 0, inode->u.romfs.seg, count);
	iput (inode);
	return count;
}


static int romfs_followlink (struct inode * dir, register struct inode * inode,
	int flag, mode_t mode, struct inode ** res_inode)
{
	int err;

	seg_t user_ds;
	seg_t *pds;

	/* It is strange that the kernel calls this function for all inodes
	 * even if they are not links... maybe something to fix here ?
	 */

	if (!S_ISLNK (inode->i_mode)) {
		*res_inode = inode;
		err = 0;
	} else {
		pds = &current->t_regs.ds;
		user_ds = *pds;
		*pds = inode->u.romfs.seg;

		/* Trick: mkromfs appends null character to link data
		 * to protect against null-terminated string comparison
		 */

		err = open_namei (0, flag, mode, res_inode, dir);
		*pds = user_ds;
	}

	return err;
}


/* Inode operations for supported types */

static struct file_operations romfs_file_operations = {
	NULL,           /* lseek - default */
	romfs_read,     /* read */
	NULL,           /* write - bad */
	romfs_readdir,  /* readdir */
	NULL,           /* select - default */
	NULL,           /* ioctl */
	NULL,           /* open */
	NULL            /* release */
};

static struct inode_operations romfs_inode_operations = {
	&romfs_file_operations,
	NULL,             /* create */
	romfs_lookup,     /* lookup */
	NULL,             /* link */
	NULL,             /* unlink */
	NULL,             /* symlink */
	NULL,             /* mkdir */
	NULL,             /* rmdir */
	NULL,             /* mknod */
	romfs_readlink,   /* readlink */
	romfs_followlink, /* followlink */
	NULL,             /* getblk */
	NULL              /* truncate */
};

static mode_t romfs_modemap [] = {
	S_IFREG,
	S_IFDIR,
	S_IFCHR,
	S_IFBLK,
	S_IFLNK
};

static struct inode_operations * romfs_inode_ops [] = {
	&romfs_inode_operations,
	&romfs_inode_operations,
	&chrdev_inode_operations,  /* standard handler */
	&blkdev_inode_operations,  /* standard handler */
	&romfs_inode_operations
};


/* Read inode from memory */

static void romfs_read_inode (struct inode * i)
{
	struct romfs_inode_mem rim;
	struct romfs_super_info * isb;
	word_t ino;
	word_t off_i;
	mode_t m;

	while (1) {
		i->i_op = NULL;

		ino = (word_t) i->i_ino - 1;  /* 0 = no inode */
		isb = &(i->i_sb->u.romfs);
		if (ino >= isb->icount) break;

		off_i = isb->ssize + ino * isb->isize;
		fmemcpyw (&rim, kernel_ds, (char *)off_i, CONFIG_ROMFS_BASE, sizeof (struct romfs_inode_mem) >> 1);

		i->i_size = rim.size;

		i->i_nlink = 1;  /* TODO: compute link count in mkromfs */
		i->i_mtime = i->i_atime = i->i_ctime = 0;
		i->i_uid = i->i_gid = 0;

		i->i_op = romfs_inode_ops [rim.flags & ROMFS_TYPE];

		/* Compute inode mode (type & permissions) */

		m = S_IRUGO | S_IXUGO | romfs_modemap [rim.flags & ROMFS_TYPE];

		if (S_ISBLK (m) || S_ISCHR (m)) {
			m = (m & ~S_IXUGO) | S_IWUGO;
			i->i_rdev = (kdev_t) rim.offset;
		}
		else {
			i->u.romfs.seg = CONFIG_ROMFS_BASE + rim.offset;
		}

		i->i_mode = m;
		break;
	}
}


/* Nothing to do */

static void romfs_put_super (struct super_block * sb)
{
	lock_super (sb);
	sb->s_dev = 0;
	unlock_super (sb);
}


/* FIXME not implemented */
static void romfs_statfs(struct super_block *s, struct statfs *sf, int flags)
{
	memset(sf, 0, sizeof(struct statfs));
	//sf->f_bsize = ROMBSIZE;
	//sf->f_blocks = (s->u.romfs.s_maxsize + ROMBSIZE - 1) >> ROMBSBITS;
}


static struct super_operations romfs_super_ops =
{
	romfs_read_inode,
	NULL,  /* write inode */
	NULL,  /* put inode */
	romfs_put_super,
	NULL,  /* write super */
	NULL,  /* remount */
	romfs_statfs
};


/* Read superblock from memory */

static struct super_block * romfs_read_super (struct super_block * s, void * data, int silent)
{
	struct super_block * res;
	struct romfs_super_mem rsm;
	struct inode * i;

	while (1) {
		lock_super (s);

		if (fmemcmpw ((void *)ROMFS_MAGIC_STR, kernel_ds, 0, CONFIG_ROMFS_BASE,
          ROMFS_MAGIC_LEN >> 1)) {
			printk ("romfs: no superblock at %x:0h\n", CONFIG_ROMFS_BASE);
			res = NULL;
			break;
		}

		fmemcpyw (&rsm, kernel_ds, 0, CONFIG_ROMFS_BASE, sizeof (struct romfs_super_mem) >> 1);
		if (rsm.ssize != sizeof (struct romfs_super_mem)) {
			printk ("romfs: superblock size mismatch\n");
			res = NULL;
			break;
		}

		s->u.romfs.ssize  = rsm.ssize;
		s->u.romfs.isize  = rsm.isize;
		s->u.romfs.icount = rsm.icount;

		/*s->s_flags |= MS_RDONLY;*/
		/*s->s_magic = ROMFS_MAGIC;*/
		s->s_op = &romfs_super_ops;

		/* Get the root inode */
		i = iget (s, 1);
		if (!i) {
			printk ("romfs: cannot get root inode\n");
			res = NULL;
			break;
		}

		s->s_mounted = i;

		res = s;
		break;
	}

	unlock_super (s);
	return res;
}


struct file_system_type romfs_fs_type = {
	romfs_read_super,
	FST_ROMFS
};
