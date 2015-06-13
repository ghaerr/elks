/*
 * ROMFS file system, Linux implementation
 *
 * Copyright (C) 1997  Janos Farkas <chexum@shadow.banki.hu>
 *
 * Using parts of the minix filesystem
 * Copyright (C) 1991, 1992  Linus Torvalds
 *
 * and parts of the affs filesystem additionally
 * Copyright (C) 1993  Ray Burr
 * Copyright (C) 1996  Hans-Joachim Widmaier
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Changes
 *					Changed for 2.1.19 modules
 *	Jan 1997			Initial release
 *
 *	Jul 1997	Michel Onstein	Started port to ELKS
 */

/* todo:
 *	use malloced memory for file names?
 *	considering write access...
 *	network (tftp) files?
 */

/* Sorry about some optimizations and for some goto's.  I just wanted
 * to squeeze some more bytes out of this code.. :)
 */

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

static void romfs_read_inode(struct inode *i);
static void romfs_put_super(struct super_block *sb);

#ifdef BLOAT_FS
static void romfs_statfs(struct super_block *sb, struct statfs *buf,
			 size_t bufsize);
#endif

static struct super_operations romfs_ops = {
    romfs_read_inode,		/* read inode */
#ifdef BLOAT_FS
    NULL,			/* notify change */
#endif
    NULL,			/* write inode */
    NULL,			/* put inode */
    romfs_put_super,		/* put super */
    NULL,			/* write super */
#ifdef BLOAT_FS
    romfs_statfs,		/* statfs */
#endif
    NULL			/* remount */
};

static __s32 romfs_checksum(void *data, size_t size)
{
    unsigned long int *ptr;
    __s32 sum;

    sum = 0;
    ptr = data;
    size >>= 2;
    while (size > 0) {
	sum += ntohl(*ptr++);
	size--;
    }
    return sum;
}

static struct super_block *romfs_read_super(struct super_block *s,
					    void *data, int silent)
{
    struct buffer_head *bh;
    kdev_t dev = s->s_dev;
    struct romfs_super_block *rsb;
    unsigned long int sz;

    /* I would parse the options, but there are none.. :) */

    lock_super(s);

    if (!(bh = bread(dev, (block_t) 0))) {
	s->s_dev = 0;
	unlock_super(s);
	printk("romfs: unable to read superblock\n");
	return NULL;
    }
    map_buffer(bh);

    rsb = (struct romfs_super_block *) bh->b_data;
    sz = ntohl(rsb->size);

    if (rsb->word0 != ROMSB_WORD0 || rsb->word1 != ROMSB_WORD1
	|| sz < ROMFH_SIZE) {
	if (!silent)
	    printk("VFS: Can't find a romfs filesystem on dev %s.\n",
		   kdevname(dev));
	goto out;
    }
    if (romfs_checksum(rsb, (size_t) min(sz, 512))) {
	printk("romfs: bad initial checksum on dev %s.\n", kdevname(dev));
    }

#ifdef BLOAT_FS
    s->s_magic = ROMFS_MAGIC;
#endif

    s->u.romfs_sb.s_maxsize = sz;

    s->s_flags |= MS_RDONLY;

    /* Find the start of the fs */
    sz = (ROMFH_SIZE + strnlen(rsb->name, ROMFS_MAXFN) + 1 + ROMFH_PAD)
	& ROMFH_MASK;

    unmap_buffer(bh);

    s->s_op = &romfs_ops;

    unlock_super(s);

    if (!(s->s_mounted = iget(s, (ino_t) sz)))
	goto outnobh;

    /* Ehrhm; sorry.. :)  And thanks to Hans-Joachim Widmaier  :) */
    if (0) {
      out:
	unmap_brelse(bh);
      outnobh:
	s->s_dev = 0;
	unlock_super(s);
	s = NULL;
    }

    return s;
}

/* Nothing to do.. */

static void romfs_put_super(struct super_block *sb)
{
    lock_super(sb);
    sb->s_dev = 0;
    unlock_super(sb);
    return;
}


/* That's simple too. */

#ifdef BLOAT_FS
#ifndef CONFIG_FS_RO

static void romfs_statfs(struct super_block *sb, struct statfs *buf,
			 size_t bufsize)
{
    struct statfs tmp;

    memset(&tmp, 0, sizeof(tmp));
    tmp.f_type = ROMFS_MAGIC;
    tmp.f_bsize = ROMBSIZE;
    tmp.f_blocks = (sb->u.romfs_sb.s_maxsize + ROMBSIZE - 1) >> ROMBSBITS;
    memcpy(buf, &tmp, bufsize);
}

#endif
#endif

static int romfs_strnlen(struct inode *i, loff_t offset, size_t count)
{
    struct buffer_head *bh;
    size_t avail, maxsize;
    int res;

    maxsize = i->i_sb->u.romfs_sb.s_maxsize;
    if (offset >= (loff_t) maxsize)
	return -1;

    /* strnlen is almost always valid */
    if (count > maxsize || offset + count > maxsize)
	count = maxsize - offset;

    bh = bread(i->i_dev, (block_t) (offset >> ROMBSBITS));
    if (!bh) {
	printk("romfs: abort: romfs_strnlen\n");
	return -1;		/* error */
    }

    map_buffer(bh);

    avail = ROMBSIZE - (offset & ROMBMASK);
    maxsize = min(count, avail);
    res = (int)
	strnlen(((char *) bh->b_data) + (offset & ROMBMASK), (size_t) maxsize);

    printk("romfs: strnlen_1: %s\n",
	   ((char *) bh->b_data) + (offset & ROMBMASK));

    unmap_brelse(bh);

    if (((size_t) res) < maxsize)
	return res;			/* found all of it */

    while (((size_t) res) < count) {
	offset += maxsize;

	bh = bread(i->i_dev, (block_t) (offset >> ROMBSBITS));
	if (!bh)
	    return -1;

	map_buffer(bh);

	maxsize = min(count - res, (long) ROMBSIZE);

	avail = strnlen(bh->b_data, (size_t) maxsize);
	printk("romfs: strnlen_2: %s\n", bh->b_data);

	res += avail;
	unmap_brelse(bh);
	if (avail < maxsize)
	    return res;
    }

    return res;
}

static int romfs_copyfrom(struct inode *i, char *dest, loff_t offset,
			  size_t count)
{
    struct buffer_head *bh;
    size_t avail, maxsize, res;

    debug("romfs: copyfrom called\n");
    debug2("romfs:     offset = 0x%x%x\n", (int) (offset >> 16), (int) offset);
    debug2("romfs:     count  = 0x%x%x\n", (int) (count  >> 16), (int) count);

    maxsize = i->i_sb->u.romfs_sb.s_maxsize;

    if (offset >= (loff_t) maxsize || count > maxsize
				   || offset + count > maxsize)
	return -1;

    debug2("romfs:     offset>>ROMBSBITS = 0x%x%x\n",
	   (int) ((offset >> ROMBSBITS) >> 16),
	   (int) (offset >> ROMBSBITS));

    bh = bread(i->i_dev, (block_t) (offset >> ROMBSBITS));

    if (!bh)
	return -1;		/* error */

    map_buffer(bh);
    avail = ROMBSIZE - (offset & ROMBMASK);
    maxsize = min(count, avail);
    memcpy(dest, ((char *) bh->b_data) + (int) (offset & ROMBMASK),
	   (size_t) maxsize);
    unmap_brelse(bh);

    res = maxsize;		/* all of it */

    while (res < count) {
	offset += maxsize;
	*dest += maxsize;

	bh = bread(i->i_dev, (block_t) (offset >> ROMBSBITS));

	if (!bh)
	    return -1;		/* error */

	map_buffer(bh);
	maxsize = min(count - res, ROMBSIZE);
	memcpy(dest, bh->b_data, (size_t) maxsize);
	unmap_brelse(bh);
	res += maxsize;
    }

    return (int) res;
}

/* Directory operations */

static int romfs_readdir(struct inode *i, struct file *filp,
			 void *dirent, filldir_t filldir)
{
    char fsname[ROMFS_MAXFN];		/* XXX dynamic? */
    struct romfs_inode ri;
    long int j;
    unsigned long int nextfh;
    loff_t offset, maxoff;
    ino_t ino;
    int stored = 0;

    debug("romfs: readdir called\n");

    if (!i || !S_ISDIR(i->i_mode))
	return -EBADF;

    maxoff = (loff_t) i->i_sb->u.romfs_sb.s_maxsize;

    offset = filp->f_pos;
    if (!offset) {
	offset = (loff_t) (i->i_ino & ROMFH_MASK);
	if (romfs_copyfrom(i, (char *)(&ri), offset, (size_t) ROMFH_SIZE) <= 0)
	    return stored;
	offset = (loff_t) ntohl(ri.spec) & ROMFH_MASK;
    }

    /* Not really failsafe, but we are read-only... */
    for (;;) {
	if (!offset || offset >= maxoff) {
	    offset = 0xffffffff;
	    filp->f_pos = offset;
	    return stored;
	}
	filp->f_pos = offset;

	/* Fetch inode info */
	if (romfs_copyfrom(i, (char *)(&ri), offset, (size_t) ROMFH_SIZE) <= 0)
	    return stored;

	j = romfs_strnlen(i, offset + ROMFH_SIZE, (size_t) sizeof(fsname) - 1);
	if (j < 0)
	    return stored;

	fsname[j] = 0;
	romfs_copyfrom(i, fsname, offset + ROMFH_SIZE, (size_t) j);

	ino = (ino_t) offset;
	nextfh = ntohl(ri.next);
	if ((nextfh & ROMFH_TYPE) == ROMFH_HRD)
	    ino = (ino_t) ntohl(ri.spec);
	if (filldir(dirent, fsname, j, offset, ino) < 0) {
	    return stored;
	}
	stored++;
	offset = (loff_t) nextfh & ROMFH_MASK;
    }
}

static int romfs_lookup(struct inode *dir, char *name, size_t len,
			struct inode **result)
{
    char fsname[ROMFS_MAXFN];	/* XXX dynamic? */
    struct romfs_inode ri;
    loff_t offset, maxoff;
    size_t fslen;
    int res;

    debug("romfs: entered lookup\n");

    *result = NULL;
    if (!dir || !S_ISDIR(dir->i_mode)) {
	res = -EBADF;
	goto out;
    }

    offset = ((loff_t) dir->i_ino) & ROMFH_MASK;

    if (romfs_copyfrom(dir, (char *)(&ri), offset, (size_t) ROMFH_SIZE) <= 0) {
	res = -ENOENT;
	goto out;
    }

    maxoff = (loff_t) dir->i_sb->u.romfs_sb.s_maxsize;
    offset = (loff_t) ntohl(ri.spec) & ROMFH_MASK;

    for (;;) {
	if (!offset || offset >= maxoff
	    || romfs_copyfrom(dir, (char *)(&ri), offset, (size_t) ROMFH_SIZE) <= 0) {
	    res = -ENOENT;
	    goto out;
	}

	/* try to match the first 16 bytes of name */
	fslen = (size_t)
	    romfs_strnlen(dir, offset + ROMFH_SIZE, (size_t) ROMFH_SIZE);
	if (len < ROMFH_SIZE) {
	    if (len == fslen) {
		/* both are shorter, and same size */
		romfs_copyfrom(dir, fsname, offset + ROMFH_SIZE,
			       (size_t) len + 1);

		if (strncmp(name, fsname, len) == 0)
		    break;
	    }
	} else if (fslen >= ROMFH_SIZE) {
	    /* both are longer; XXX optimize max size */
	    fslen = (size_t)
		romfs_strnlen(dir, offset + ROMFH_SIZE,
			      (size_t) sizeof(fsname) - 1);
	    if (len == fslen) {
		romfs_copyfrom(dir, fsname, offset + ROMFH_SIZE,
			       (size_t) len + 1);

		if (strncmp(name, fsname, len) == 0)
		    break;
	    }
	}
	/* next entry */
	offset = (loff_t) ntohl(ri.next) & ROMFH_MASK;
    }

    /* Hard link handling */
    if ((ntohl(ri.next) & ROMFH_TYPE) == ROMFH_HRD)
	offset = (loff_t) ntohl(ri.spec) & ROMFH_MASK;

    res = 0;
    if (!(*result = iget(dir->i_sb, (ino_t) offset)))
	res = -EACCES;

  out:
    iput(dir);
    return res;
}

#if 0
/*
 * Ok, we do readpage, to be able to execute programs.  Unfortunately,
 * bmap is not applicable, since we have looser alignments.
 *
 * XXX I'm not quite sure that I need to muck around the PG_xx bits..
 */

static int romfs_readpage(struct inode *inode, struct page *page)
{

#if 0
    unsigned long buf;
    unsigned long offset, avail, readlen;
    int result = -EIO;

    buf = page_address(page);
    atomic_inc(&page->count);
    offset = page->offset;
    if (offset < inode->i_size) {
	avail = inode->i_size - offset;
	readlen = min(avail, PAGE_SIZE);
	if (romfs_copyfrom
	    (inode, (void *) buf, inode->u.romfs_i.i_dataoffset + offset,
	     readlen) == readlen) {
	    if (readlen < PAGE_SIZE) {
		memset((void *) (buf + readlen), 0, PAGE_SIZE - readlen);
	    }
	    result = 0;
	    set_bit(PG_uptodate, &page->flags);
	} else {
	    memset((void *) buf, 0, PAGE_SIZE);
	}
    }
    free_page(buf);
    return result;
#endif

}
#endif

static int romfs_readlink(struct inode *inode, char *buffer, size_t len)
{
    char buf[ROMFS_MAXFN];	/* XXX dynamic */
    size_t mylen;

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
}

/* Mapping from our types to the kernel */

static struct file_operations romfs_file_operations = {
    NULL,			/* lseek - default */
#if 0
    generic_file_read,		/* read */
#else
    NULL,			/* read */
#endif
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
    romfs_readdir,		/* readdir */
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
    0, S_IFDIR, S_IFREG, S_IFLNK + 0777,
    S_IFBLK, S_IFCHR, S_IFSOCK, S_IFIFO
};

static struct inode_operations *romfs_inoops[] = {
    NULL,			/* hardlink, handled elsewhere */
    &romfs_dirlink_inode_operations,
    &romfs_file_inode_operations,
    &romfs_dirlink_inode_operations,
    &blkdev_inode_operations,	/* standard handlers */
    &chrdev_inode_operations,
    NULL,			/* socket */
    NULL			/* fifo */
};

static void romfs_read_inode(struct inode *i)
{
    struct romfs_inode ri;
    unsigned long nextfh;
    ino_t ino;

    i->i_op = NULL;

    ino = i->i_ino & ROMFH_MASK;

    printk("romfs: start looking for inode 0x%x%x\n",
	   (int) (ino >> 16), (int) ino);

    /* Loop for finding the real hard link */
    for (;;) {
	if (romfs_copyfrom(i, (char *)(&ri), (loff_t) ino, (size_t) ROMFH_SIZE) <= 0) {
	    printk("romfs: read error for inode 0x%x%x\n",
		   (int) (ino >> 16), (int) ino);
	    return;
	}

	nextfh = ntohl(ri.next);

	if ((nextfh & ROMFH_TYPE) != ROMFH_HRD)
	    break;

	ino = (ino_t) ntohl(ri.spec) & ROMFH_MASK;
    }

    i->i_nlink = 1;		/* Hard to decide.. */
    i->i_size = ntohl(ri.size);
    i->i_mtime = i->i_atime = i->i_ctime = 0;
    i->i_uid = i->i_gid = 0;

    i->i_op = romfs_inoops[nextfh & ROMFH_TYPE];

    /* Precalculate the data offset */
    ino = (ino_t) romfs_strnlen(i, ino + ROMFH_SIZE, (size_t) ROMFS_MAXFN);
    if (ino >= 0)
	ino = ((ROMFH_SIZE + ino + 1 + ROMFH_PAD) & ROMFH_MASK);
    else
	ino = 0;
    i->u.romfs_i.i_metasize = (unsigned long int) ino;
    i->u.romfs_i.i_dataoffset = ino + (i->i_ino & ROMFH_MASK);

    /* Compute permissions */
    ino = (ino_t) (S_IRUGO | S_IWUSR);
    ino |= romfs_modemap[nextfh & ROMFH_TYPE];
    if (nextfh & ROMFH_EXEC)
	ino |= S_IXUGO;
    i->i_mode = (__u16) ino;

#ifdef NOT_YET
    if (S_ISFIFO(ino))
	init_fifo(i);
#endif

    if (S_ISDIR(ino))
	i->i_size = i->u.romfs_i.i_metasize;
    else if (S_ISBLK(ino) || S_ISCHR(ino)) {
	i->i_mode &= ~(S_IRWXG | S_IRWXO);
	ino = (ino_t) ntohl(ri.spec);
	i->i_rdev = (kdev_t) MKDEV(ino >> 16, ino & 0xffff);
    }

    debug("romfs: got inode\n");
}

struct file_system_type romfs_fs_type = {
    romfs_read_super,
    "romfs"
#ifdef BLOAT_FS
		,
    1
#endif
};

int init_romfs_fs(void)
{
    return 1;
}
