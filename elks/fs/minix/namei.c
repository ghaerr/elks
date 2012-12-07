/*
 *  linux/fs/minix/namei.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/minix_fs.h>
#include <linuxmt/kernel.h>
#include <linuxmt/string.h>
#include <linuxmt/stat.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/errno.h>
#include <linuxmt/debug.h>
#include <linuxmt/mm.h>

#include <arch/segment.h>

/*
 * comment out this line if you want names > info->s_namelen chars to be
 * truncated. Else they will be disallowed (ENAMETOOLONG).
 */
/* #define NO_TRUNCATE */

static int namecompare(size_t len, size_t max, char *name, register char *buf)
{
    return ((len < max) && (buf[len] != 0))
	? 0 : !fs_memcmp(name, buf, len);
}

/*
 * ok, we cannot use strncmp, as the name is not in our data space.
 * Thus we'll have to use minix_match. No big problem. Match also makes
 * some sanity tests.
 *
 * NOTE! unlike strncmp, minix_match returns 1 for success, 0 for failure.
 *
 * Note2: bh must already be mapped! 
 */
static int minix_match(size_t len,
		       char *name,
		       struct buffer_head *bh,
		       loff_t * offset, register struct minix_sb_info *info)
{
    register struct minix_dir_entry *de;

    de = (struct minix_dir_entry *) (bh->b_data + *offset);
    *offset += info->s_dirsize;
    if (!de->inode || len > info->s_namelen) {
	return 0;
    }
    /* "" means "." ---> so paths like "/usr/lib//libc.a" work */
    if (!len && (de->name[0] == '.') && (de->name[1] == '\0')) {
	return 1;
    }
    return namecompare(len, info->s_namelen, name, de->name);
}

/*
 *	minix_find_entry()
 *
 * finds an entry in the specified directory with the wanted name. It
 * returns the cache buffer in which the entry was found, and the entry
 * itself (as a parameter - res_dir). It does NOT read the inode of the
 * entry - you'll have to do that yourself if you want to.
 * 
 */

static struct buffer_head *minix_find_entry(register struct inode *dir,
					    char *name, size_t namelen,
					    struct minix_dir_entry **res_dir)
{
    struct buffer_head *bh;
    struct minix_sb_info *info;
    block_t block;
    loff_t offset;

    *res_dir = NULL;
    if (!dir || !dir->i_sb)
	return NULL;
    info = &dir->i_sb->u.minix_sb;
    if (namelen > info->s_namelen) {

#ifdef NO_TRUNCATE
	return NULL;
#else
	namelen = info->s_namelen;
#endif

    }
    bh = NULL;
    block = 0;
    offset = 0L;
    while (block * BLOCK_SIZE + offset < dir->i_size) {
	if (!bh) {
	    bh = minix_bread(dir, block, 0);
	    if (!bh) {
		block++;
		continue;
	    }
	}
	map_buffer(bh);
	*res_dir = (struct minix_dir_entry *) (bh->b_data + offset);

	if (minix_match(namelen, name, bh, &offset, info)) {
	    unmap_buffer(bh);
	    return bh;
	}
	unmap_buffer(bh);
	if (offset < 1024)
	    continue;
	brelse(bh);
	bh = NULL;
	offset = 0;
	block++;
    }
    brelse(bh);
    *res_dir = NULL;
    return NULL;
}

int minix_lookup(register struct inode *dir, char *name, size_t len,
		 register struct inode **result)
{
    unsigned int ino;
    struct minix_dir_entry *de;
    struct buffer_head *bh;

    *result = NULL;

    if (dir) {
	if (S_ISDIR(dir->i_mode)) {
	    debug("minix_lookup: Entering minix_find_entry\n");
	    bh = minix_find_entry(dir, name, len, &de);
	    debug2("minix_lookup: minix_find_entry returned %x %d\n", bh,
		   bh->b_mapcount);
	    if (bh) {
		map_buffer(bh);
		ino = de->inode;
		unmap_brelse(bh);
		*result = iget(dir->i_sb, (ino_t) ino);
		iput(dir);
		return (!*result) ? -EACCES : 0;
	    }
	}
	iput(dir);
    }
    return -ENOENT;
}

/*
 *	minix_add_entry()
 *
 * adds a file entry to the specified directory, returning a possible
 * error value if it fails.
 *
 * NOTE!! The inode part of 'de' is left at 0 - which means you
 * may not sleep between calling this and putting something into
 * the entry, as someone else might have used it while you slept.
 */

static int minix_add_entry(register struct inode *dir,
			   char *name,
			   size_t namelen,
			   struct buffer_head **res_buf,
			   struct minix_dir_entry **res_dir)
{
    unsigned short block;
    loff_t offset;
    register struct buffer_head *bh;
    struct minix_dir_entry *de;
    struct minix_sb_info *info;

    *res_buf = NULL;
    *res_dir = NULL;
    if (!dir || !dir->i_sb)
	return -ENOENT;
    info = &dir->i_sb->u.minix_sb;
    if (namelen > info->s_namelen) {
#ifdef NO_TRUNCATE
	return -ENAMETOOLONG;
#else
	namelen = info->s_namelen;
#endif
    }
    if (!namelen)
	return -ENOENT;
    bh = NULL;
    block = 0;
    offset = 0L;
    while (1) {
	if (!bh) {
	    bh = minix_bread(dir, block, 1);
	    if (!bh)
		return -ENOSPC;
	    map_buffer(bh);
	}
	de = (struct minix_dir_entry *) (bh->b_data + offset);
	offset += info->s_dirsize;
	if (block * 1024L + offset > dir->i_size) {
	    de->inode = 0;
	    dir->i_size = block * 1024L + offset;
	    dir->i_dirt = 1;
	}
	if (de->inode) {
	    if (namecompare(namelen, info->s_namelen, name, de->name)) {
		debug2("MINIXadd_entry: file %t==%s (already exists)\n",
		     name, de->name);
		unmap_brelse(bh);
		return -EEXIST;
	    }
	} else {
	    size_t i;

	    dir->i_mtime = dir->i_ctime = CURRENT_TIME;
	    dir->i_dirt = 1;
	    for (i = 0; i < info->s_namelen; i++)
		de->name[i] = (i < namelen) ? (char) get_user_char(name + i)
					    : '\0';

#ifdef BLOAT_FS
	    dir->i_version = ++event;
#endif

	    unmap_buffer(bh);
	    mark_buffer_dirty(bh, 1);
	    *res_dir = de;
	    break;
	}
	if (offset < 1024)
	    continue;
	unmap_brelse(bh);
	bh = NULL;
	offset = 0;
	block++;
    }
    *res_buf = bh;
    return 0;
}

int minix_create(register struct inode *dir, char *name, size_t len,
		 int mode, struct inode **result)
{
    register struct inode *inode;
    struct buffer_head *bh;
    struct minix_dir_entry *de;
    int error;

    *result = NULL;
    if (!dir)
	return -ENOENT;
    inode = minix_new_inode(dir);
    if (!inode) {
	iput(dir);
	return -ENOSPC;
    }
    inode->i_op = &minix_file_inode_operations;
    inode->i_mode = (__u16) mode;
    inode->i_dirt = 1;
    error = minix_add_entry(dir, name, len, &bh, &de);
    if (error) {
	inode->i_nlink--;
	inode->i_dirt = 1;
	iput(inode);
	iput(dir);
	return error;
    }
    de->inode = inode->i_ino;
    mark_buffer_dirty(bh, 1);
    brelse(bh);
    iput(dir);
    *result = inode;
    return 0;
}

int minix_mknod(register struct inode *dir, char *name, size_t len,
		int mode, int rdev)
{
    int error;
    register struct inode *inode;
    struct buffer_head *bh;
    struct minix_dir_entry *de;

    if (!dir)
	return -ENOENT;
    bh = minix_find_entry(dir, name, len, &de);
    if (bh) {
	brelse(bh);
	iput(dir);
	return -EEXIST;
    }
    inode = minix_new_inode(dir);
    if (!inode) {
	iput(dir);
	return -ENOSPC;
    }
    inode->i_uid = current->euid;
    inode->i_mode = (__u16) mode;
    inode->i_op = NULL;

    minix_set_ops(inode);

    if (S_ISDIR(inode->i_mode))
	if (dir->i_mode & S_ISGID)
	    inode->i_mode |= S_ISGID;

    if (S_ISBLK(mode) || S_ISCHR(mode))
	inode->i_rdev = to_kdev_t(rdev);
    inode->i_dirt = 1;
    error = minix_add_entry(dir, name, len, &bh, &de);
#if 1
    if (error) {
	inode->i_nlink--;
	inode->i_dirt = 1;
	iput(inode);
	iput(dir);
	return error;
    }
    de->inode = inode->i_ino;
    mark_buffer_dirty(bh, 1);
    brelse(bh);
    iput(dir);
    iput(inode);
    return 0;
#else  /* MJN3: If the ordering of the iput()s isn't important, do this. */
    if (error) {
	inode->i_nlink--;
	inode->i_dirt = 1;
    } else {
	de->inode = inode->i_ino;
	mark_buffer_dirty(bh, 1);
	brelse(bh);
    }
    iput(dir);
    iput(inode);
    return error;
#endif
}

int minix_mkdir(register struct inode *dir, char *name, size_t len, int mode)
{
    int error;
    register struct inode *inode;
    struct buffer_head *dir_block;
    struct buffer_head *bh;
    struct minix_dir_entry *de;

    if (!dir || !dir->i_sb) {
	iput(dir);
	return -EINVAL;
    }
    bh = minix_find_entry(dir, name, len, &de);
    if (bh) {
	brelse(bh);
	iput(dir);
	return -EEXIST;
    }
    if (dir->i_nlink >= MINIX_LINK_MAX) {
	iput(dir);
	return -EMLINK;
    }
    inode = minix_new_inode(dir);
    if (!inode) {
	iput(dir);
	return -ENOSPC;
    }
    debug("m_mkdir: new_inode succeeded\n");
    inode->i_op = &minix_dir_inode_operations;
    inode->i_size = 2 * dir->i_sb->u.minix_sb.s_dirsize;
    debug("m_mkdir: starting minix_bread\n");
    dir_block = minix_bread(inode, 0, 1);
    if (!dir_block) {
	iput(dir);
	inode->i_nlink--;
	inode->i_dirt = 1;
	iput(inode);
	return -ENOSPC;
    }
    debug("m_mkdir: read succeeded\n");
    map_buffer(dir_block);
    de = (struct minix_dir_entry *) dir_block->b_data;
    de->inode = inode->i_ino;
    strcpy(de->name, ".");
    de =
	(struct minix_dir_entry *) (dir_block->b_data +
				    dir->i_sb->u.minix_sb.s_dirsize);
    de->inode = dir->i_ino;
    strcpy(de->name, "..");
    inode->i_nlink = 2;
    mark_buffer_dirty(dir_block, 1);
    unmap_brelse(dir_block);
    debug("m_mkdir: dir_block update succeeded\n");
    inode->i_mode = S_IFDIR | (mode & 0777 & ~current->fs.umask);
    if (dir->i_mode & S_ISGID)
	inode->i_mode |= S_ISGID;
    inode->i_dirt = 1;
    error = minix_add_entry(dir, name, len, &bh, &de);
    if (error) {
	iput(dir);
	inode->i_nlink = 0;
	iput(inode);
	return error;
    }
    map_buffer(bh);
    de->inode = inode->i_ino;
    mark_buffer_dirty(bh, 1);
    dir->i_nlink++;
    dir->i_dirt = 1;
    iput(dir);
    iput(inode);
    unmap_brelse(bh);
    debug("m_mkdir: done!\n");
    return 0;
}

/*
 * routine to check that the specified directory is empty (for rmdir)
 */
static int empty_dir(register struct inode *inode)
{
    unsigned short block;
    loff_t offset;
    register struct buffer_head *bh;
    struct minix_dir_entry *de;

    if (!inode || !inode->i_sb)
	return 1;
    block = 0;
    bh = NULL;
    offset = 2 * inode->i_sb->u.minix_sb.s_dirsize;
    if (inode->i_size & (inode->i_sb->u.minix_sb.s_dirsize - 1))
	goto bad_dir;
    if (inode->i_size < (__u32) offset)
	goto bad_dir;
    bh = minix_bread(inode, 0, 0);
    if (!bh)
	goto bad_dir;
    map_buffer(bh);
    de = (struct minix_dir_entry *) bh->b_data;
    if (!de->inode || strcmp(de->name, "."))
	goto bad_dir;
    de =
	(struct minix_dir_entry *) (bh->b_data +
				    inode->i_sb->u.minix_sb.s_dirsize);
    if (!de->inode || strcmp(de->name, ".."))
	goto bad_dir;
    while (block * 1024L + offset < inode->i_size) {
	if (!bh) {
	    bh = minix_bread(inode, block, 0);
	    if (!bh) {
		block++;
		continue;
	    }
	}
	de = (struct minix_dir_entry *) (bh->b_data + offset);
	offset += inode->i_sb->u.minix_sb.s_dirsize;
	if (de->inode) {
	    unmap_brelse(bh);
	    return 0;
	}
	if (offset < 1024L)
	    continue;
	unmap_brelse(bh);
	bh = NULL;
	offset = 0L;
	block++;
    }
    brelse(bh);
    return 1;
  bad_dir:
    unmap_brelse(bh);
    printk("Bad directory on device %s\n", kdevname(inode->i_dev));
    return 1;
}

int minix_rmdir(register struct inode *dir, char *name, size_t len)
{
    int retval;
    register struct inode *inode;
    struct buffer_head *bh;
    struct minix_dir_entry *de;

    inode = NULL;
    bh = minix_find_entry(dir, name, len, &de);
    retval = -ENOENT;
    if (!bh)
	goto end_rmdir;
    map_buffer(bh);
    retval = -EPERM;
    if (!(inode = iget(dir->i_sb, (ino_t) de->inode)))
	goto end_rmdir;
    if ((dir->i_mode & S_ISVTX) && !suser() &&
	current->euid != inode->i_uid && current->euid != dir->i_uid)
	goto end_rmdir;
    if (inode->i_dev != dir->i_dev)
	goto end_rmdir;
    if (inode == dir)		/* we may not delete ".", but "../dir" is ok */
	goto end_rmdir;
    if (!S_ISDIR(inode->i_mode)) {
	retval = -ENOTDIR;
	goto end_rmdir;
    }
    if (!empty_dir(inode)) {
	retval = -ENOTEMPTY;
	goto end_rmdir;
    }
    if (de->inode != inode->i_ino) {
	retval = -ENOENT;
	goto end_rmdir;
    }
    if (inode->i_count > 1) {
	retval = -EBUSY;
	goto end_rmdir;
    }
    if (inode->i_nlink != 2)
	printk("empty directory has nlink!=2 (%u)\n", inode->i_nlink);
    de->inode = 0;

#ifdef BLOAT_FS
    dir->i_version = ++event;
#endif

    mark_buffer_dirty(bh, 1);
    inode->i_nlink = 0;
    inode->i_dirt = 1;
    inode->i_ctime = dir->i_ctime = dir->i_mtime = CURRENT_TIME;
    dir->i_nlink--;
    dir->i_dirt = 1;
    retval = 0;

  end_rmdir:
    iput(dir);
    iput(inode);
    unmap_brelse(bh);
    return retval;
}

int minix_unlink(struct inode *dir, char *name, size_t len)
{
    int retval;
    register struct inode *inode;
    struct buffer_head *bh;
    struct minix_dir_entry *de;

  repeat:
    retval = -ENOENT;
    inode = NULL;
    bh = minix_find_entry(dir, name, len, &de);
    if (!bh)
	goto end_unlink;
    map_buffer(bh);
    if (!(inode = iget(dir->i_sb, (ino_t) de->inode)))
	goto end_unlink;
    retval = -EPERM;
    if (S_ISDIR(inode->i_mode))
	goto end_unlink;
    if (de->inode != inode->i_ino) {
	iput(inode);
	unmap_brelse(bh);

	schedule();
	goto repeat;
    }
    if ((dir->i_mode & S_ISVTX) && !suser() &&
	current->euid != inode->i_uid && current->euid != dir->i_uid)
	goto end_unlink;
    if (de->inode != inode->i_ino) {
	retval = -ENOENT;
	goto end_unlink;
    }
    if (!inode->i_nlink) {
	printk("Deleting nonexistent file (%s:%lu), %u\n",
	       kdevname(inode->i_dev), inode->i_ino, inode->i_nlink);
	inode->i_nlink = 1;
    }
    de->inode = 0;

#ifdef BLOAT_FS
    dir->i_version = ++event;
#endif

    mark_buffer_dirty(bh, 1);
    dir->i_ctime = dir->i_mtime = CURRENT_TIME;
    dir->i_dirt = 1;
    inode->i_nlink--;
    inode->i_ctime = dir->i_ctime;
    inode->i_dirt = 1;
    retval = 0;

  end_unlink:
    unmap_brelse(bh);
    iput(inode);
    iput(dir);
    return retval;
}

int minix_symlink(struct inode *dir, char *name, size_t len, char *symname)
{
    struct minix_dir_entry *de;
    register struct inode *inode = NULL;
    struct buffer_head *bh = NULL;
    register struct buffer_head *name_block = NULL;
    int i;
    char c;

    if (!(inode = minix_new_inode(dir))) {
	iput(dir);
	return -ENOSPC;
    }
    inode->i_mode = (__u16) (S_IFLNK | 0777);
    inode->i_op = &minix_symlink_inode_operations;
    name_block = minix_bread(inode, 0, 1);
    if (!name_block) {
	iput(dir);
	inode->i_nlink--;
	inode->i_dirt = 1;
	iput(inode);
	return -ENOSPC;
    }
    map_buffer(name_block);
    i = 0;
    while (i < 1023 && (c = *(symname++)))
	name_block->b_data[i++] = c;
    name_block->b_data[i] = 0;
    mark_buffer_dirty(name_block, 1);
    unmap_brelse(name_block);
    inode->i_size = (__u32) i;
    inode->i_dirt = 1;
    bh = minix_find_entry(dir, name, len, &de);
    map_buffer(bh);
    if (bh) {
	inode->i_nlink--;
	inode->i_dirt = 1;
	iput(inode);
	unmap_brelse(bh);
	iput(dir);
	return -EEXIST;
    }
    i = minix_add_entry(dir, name, len, &bh, &de);
    if (i) {
	inode->i_nlink--;
	inode->i_dirt = 1;
	iput(inode);
	iput(dir);
	return i;
    }
    de->inode = inode->i_ino;
    mark_buffer_dirty(bh, 1);
    brelse(bh);
    iput(dir);
    iput(inode);
    return 0;
}

int minix_link(register struct inode *oldinode, register struct inode *dir,
		char *name, size_t len)
{
    int error;
    struct minix_dir_entry *de;
    struct buffer_head *bh;

    if (S_ISDIR(oldinode->i_mode)) {
	iput(oldinode);
	iput(dir);
	return -EPERM;
    }
    if (oldinode->i_nlink >= MINIX_LINK_MAX) {
	iput(oldinode);
	iput(dir);
	return -EMLINK;
    }
    bh = minix_find_entry(dir, name, len, &de);
    if (bh) {
	brelse(bh);
	iput(dir);
	iput(oldinode);
	return -EEXIST;
    }
    error = minix_add_entry(dir, name, len, &bh, &de);
    if (error) {
	iput(dir);
	iput(oldinode);
	return error;
    }
    de->inode = oldinode->i_ino;
    mark_buffer_dirty(bh, 1);
    brelse(bh);
    iput(dir);
    oldinode->i_nlink++;
    oldinode->i_ctime = CURRENT_TIME;
    oldinode->i_dirt = 1;
    iput(oldinode);
    return 0;
}
