/* ROMFS - A tiny filesystem in ROM */

#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/romfs_fs.h>
#include <linuxmt/fs.h>
#include <linuxmt/locks.h>
#include <linuxmt/stat.h>
#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/string.h>
#include <linuxmt/debug.h>

#include <arch/border.h>

#define min(a,b)	( (a) < (b) ? (a) : (b) )


#ifdef BLOAT_FS
static void romfs_statfs(struct super_block *sb, struct statfs *buf,
			 size_t bufsize);
#endif

/* That's simple too. */

#ifdef BLOAT_FS
#ifndef CONFIG_FS_RO

static void romfs_statfs(struct super_block *sb, struct statfs *buf,
			 size_t bufsize)
{
    struct statfs tmp;

    printk ("romfs: statfs()\n");

    memset(&tmp, 0, sizeof(tmp));
    tmp.f_type = ROMFS_MAGIC;
    tmp.f_bsize = ROMBSIZE;
    tmp.f_blocks = (sb->u.romfs_sb.s_maxsize + ROMBSIZE - 1) >> ROMBSBITS;
    memcpy(buf, &tmp, bufsize);
}

#endif
#endif


/* File operations */

static size_t romfs_read (struct inode * inode, struct file * filp, char * buf, size_t len)
	{
	size_t count;
	struct romfs_inode_s ri;
	int res;
	seg_t ds;

	while (1)
		{
		ino_t ino = inode->i_ino;
		res = romfs_inode_get (ino, &ri);
		if (res)
			{
			printk ("romfs: cannot read inode 0x%\n", (int) ino);
			count = -1;
			break;
			}

		/* ELKS trick: the destination buffer is in the current task data segment */
		ds = current->t_regs.ds;

		res = romfs_file_read (ri.offset, (word_t) filp->f_pos, ds, buf, (word_t) len);
		if (res)
			{
			printk ("romfs: cannot read file\n");
			count = -1;
			break;
			}

		filp->f_pos += len;
		count = len;
		break;
		}

	return count;
	}


/* Directory operations */

static int romfs_readdir (struct inode *i, struct file *filp,
	void *dirent, filldir_t filldir)
	{
    printk ("romfs: readdir()\n");
	/*
    char fsname[ROMFS_MAXFN];
    struct romfs_inode_s ri;
    long int j;
    unsigned long int nextfh;
    loff_t offset, maxoff;
    ino_t ino;
    int stored = 0;

    printk ("romfs: readdir()\n");

    if (!i || !S_ISDIR(i->i_mode)) return -EBADF;

    maxoff = (loff_t) i->i_sb->u.romfs_sb.s_maxsize;

    offset = filp->f_pos;
    if (!offset) {
	offset = (loff_t) (i->i_ino & ROMFH_MASK);
	if (romfs_copyfrom(i, (char *)(&ri), offset, (size_t) ROMFH_SIZE) <= 0)
	    return stored;
	offset = (loff_t) ntohl(ri.spec) & ROMFH_MASK;
    }

    /* Not really failsafe, but we are read-only... */
    /*for (;;) {
	if (!offset || offset >= maxoff) {
	    offset = 0xffffffff;
	    filp->f_pos = offset;
	    return stored;
	}
	filp->f_pos = offset;*/

	/* Fetch inode info */
	/*if (romfs_copyfrom(i, (char *)(&ri), offset, (size_t) ROMFH_SIZE) <= 0)
	    return stored;

	j = romfs_strnlen(i, offset + ROMFH_SIZE, (size_t) sizeof(fsname) - 1);
	if (j < 0)
	    return stored;

	fsname[j] = 0;
	romfs_copyfrom(i, fsname, offset + ROMFH_SIZE, (size_t) j);

	ino = (ino_t) offset;
	nextfh = ntohl(ri.next);
	if ((nextfh & ROMFH_TYPE) == ROMFH_HRD) ino = (ino_t) ntohl(ri.spec);
	if (filldir(dirent, fsname, j, offset, ino) < 0) return stored;
	stored++;
	offset = (loff_t) nextfh & ROMFH_MASK;
    }*/

	return -1;
	}


static int romfs_lookup (struct inode * dir, char *name, size_t len,
	struct inode **result)
	{
	int res;
	struct romfs_inode_s ri;
	ino_t ino;
	struct inode * i;
	seg_t ds;

	while (1)
		{
		ino = dir->i_ino;
		res = romfs_inode_get (ino, &ri);
		if (res)
			{
			printk ("romfs: cannot read inode 0x%\n", (int) ino);
			res = -1;
			break;
			}

		/* ELKS trick: the name is in the current task data segment */
		ds = current->t_regs.ds;

		ino = 0;
		res = romfs_dir_lookup (ri.offset, ds, name, (byte_t) len, &ino);
		if (res)
			{
			res = -ENOENT;
			break;
			}

		i = iget (dir->i_sb, ino);
		if (!i)
			{
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


static int romfs_readlink (struct inode *inode, char *buffer, size_t len)
{
    char buf[ROMFS_MAXFN];	/* XXX dynamic */
    size_t mylen;

    printk ("romfs: readlink()\n");

    return -1;

    /*
    if (!inode || !S_ISLNK(inode->i_mode)) {
	iput(inode);
	return -EBADF;
    }

    mylen = min(sizeof(buf), inode->i_size);

    if (romfs_copyfrom
	(inode, buf, (loff_t) inode->u.romfs_i.i_dataoffset, mylen) <= 0) {
	iput(inode);
	return -EIO;
    }
    memcpy_tofs(buffer, buf, mylen);
    iput(inode);
    return (int) mylen;
    */
}

/* Mapping from our types to the kernel */

static struct file_operations romfs_file_operations = {
    NULL,			/* lseek - default */
    romfs_read,		/* read */
    NULL,			/* write - bad */
    NULL,			/* readdir */
    NULL,			/* select - default */
    NULL,			/* ioctl */
    NULL,			/* open */
    NULL			/* release */
#ifdef BLOAT_FS
	,
    NULL,			/* fsync */
    NULL,			/* check_media_change */
    NULL			/* revalidate */
#endif
};

static struct inode_operations romfs_file_inode_operations = {
    &romfs_file_operations,
    NULL,			/* create */
    NULL,			/* lookup */
    NULL,			/* link */
    NULL,			/* unlink */
    NULL,			/* symlink */
    NULL,			/* mkdir */
    NULL,			/* rmdir */
    NULL,			/* mknod */
    NULL,			/* readlink */
    NULL,			/* followlink */
#ifdef BLOAT_FS
    NULL,			/* bmap -- not really */
#endif
    NULL			/* truncate */
#ifdef BLOAT_FS
	,
    NULL			/* permission */
#endif
};

static struct file_operations romfs_dir_operations = {
    NULL,			/* lseek - default */
    NULL,			/* read */
    NULL,			/* write - bad */
    romfs_readdir,	/* readdir */
    NULL,			/* select - default */
    NULL,			/* ioctl */
    NULL,			/* open */
    NULL			/* release */
#ifdef BLOAT_FS
	,
    NULL,			/* fsync */
    NULL,			/* check_media_change */
    NULL			/* revalidate */
#endif
};

/* Merged dir/symlink op table.  readdir/lookup/readlink
 * will protect from type mismatch.
 */

static struct inode_operations romfs_dirlink_inode_operations = {
    &romfs_dir_operations,
    NULL,			/* create */
    romfs_lookup,		/* lookup */
    NULL,			/* link */
    NULL,			/* unlink */
    NULL,			/* symlink */
    NULL,			/* mkdir */
    NULL,			/* rmdir */
    NULL,			/* mknod */
    romfs_readlink,		/* readlink */
    NULL,			/* followlink */
#ifdef BLOAT_FS
    NULL,			/* bmap */
#endif
    NULL			/* truncate */
#ifdef BLOAT_FS
	,
    NULL			/* permission */
#endif
};


static mode_t romfs_modemap[] = {
    0, S_IFREG, S_IFDIR, S_IFLNK + 0777,
    S_IFBLK, S_IFCHR, S_IFSOCK, S_IFIFO
};


static struct inode_operations *romfs_inode_ops[] = {
    NULL,			/* hardlink, handled elsewhere */
    &romfs_file_inode_operations,
    &romfs_dirlink_inode_operations,
    &romfs_dirlink_inode_operations,
    &blkdev_inode_operations,	/* standard handlers */
    &chrdev_inode_operations,
    NULL,			/* socket */
    NULL			/* fifo */
};


static void romfs_read_inode (struct inode * i)
	{
	struct romfs_inode_s ri;
	ino_t ino;
	int err;

	while (1)
		{
		i->i_op = NULL;

		ino = i->i_ino;
		err = romfs_inode_get (ino, &ri);
		if (err)
			{
			printk ("romfs: cannot read inode 0x%\n", (int) ino);
			break;
			}

		i->i_nlink = 1;		/* TODO: compute link count in mkromfs */
		i->i_size = ri.size;
		i->i_mtime = i->i_atime = i->i_ctime = 0;
		i->i_uid = i->i_gid = 0;

		i->i_op = romfs_inode_ops [ri.flags & ROMFH_TYPE];

		/* Compute inode mode (type & permissions) */

		ino = (ino_t) (S_IRUGO | S_IXUGO);
		ino |= romfs_modemap [ri.flags & ROMFH_TYPE];
		i->i_mode = (__u16) ino;

#ifdef NOT_YET
		if (S_ISFIFO(ino)) init_fifo(i);
#endif

		if (S_ISDIR(ino)) i->i_size = 1 /*i->u.romfs_i.i_metasize*/; /* what's that ? */
		else if (S_ISBLK(ino) || S_ISCHR(ino))
			{
			i->i_mode &= ~(S_IRWXG | S_IRWXO);
			/*ino = (ino_t) ntohl(ri.spec);*/
			i->i_rdev = (kdev_t) MKDEV(ino >> 16, ino & 0xffff);
			}

		break;
		}
	}


/* Nothing to do */

static void romfs_put_super (struct super_block * sb)
	{
	printk ("romfs: put_super()\n");
	lock_super (sb);
	sb->s_dev = 0;
	unlock_super (sb);
	}


static struct super_operations romfs_ops =
	{
	romfs_read_inode,
#ifdef BLOAT_FS
	NULL,  /* notify change */
#endif
	NULL,  /* write inode */
	NULL,  /* put inode */
	romfs_put_super,
	NULL,  /* write super */
#ifdef BLOAT_FS
	romfs_statfs,
#endif
	NULL   /* remount */
	};


/* Get superblock in ROM */
/* No device needed */

static struct super_block * romfs_read_super (struct super_block * s, void * data, int silent)
	{
	struct super_block * res;
	struct romfs_superblock_s rsb;
	struct inode * i;
	int err;

	while (1)
		{
		lock_super (s);
		err = romfs_super_get (&rsb);
		if (err)
			{
			printk ("romfs: cannot get superblock\n");
			res = NULL;
			break;
			}

		s->s_flags |= MS_RDONLY;
		s->s_op = &romfs_ops;

#ifdef BLOAT_FS
		s->s_magic = ROMFS_MAGIC;
#endif

		/* Check the root inode */

		i = iget (s, 0);
		if (!i)
			{
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


struct file_system_type romfs_fs_type =
	{
	romfs_read_super,
	"romfs"
#ifdef BLOAT_FS
	,1
#endif
	};
