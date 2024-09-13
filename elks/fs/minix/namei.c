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

static int namecompare(size_t len, size_t max, const char *name, register char *buf)
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
		       const char *name,
		       register struct minix_dir_entry *de,
		       size_t minixlen)
{
    if (!de->inode) {
	return 0;
    }
    /* "" means "." ---> so paths like "/usr/lib//libc.a" work */
    if (!len && (de->name[0] == '.') && (de->name[1] == '\0')) {
	return 1;
    }
    return namecompare(len, minixlen, name, de->name);
}

/*
 *	minix_find_entry()
 *
 * finds an entry in the specified directory with the wanted name. It
 * returns the cache buffer in which the entry was found, and the entry
 * itself (as a parameter - res_dir). It does NOT read the inode of the
 * entry - you'll have to do that yourself if you want to.
 *
 * If succesful, the buffer cache returns MAPPED, otherwise returns NULL
 *
 */

static struct buffer_head *minix_find_entry(register struct inode *dir,
					    const char *name, size_t namelen,
					    struct minix_dir_entry **res_dir)
{
    register struct buffer_head *bh;
    struct minix_sb_info *info;
    __u32 bo;
    unsigned short offset;

    *res_dir = NULL;
    if (!dir || !dir->i_sb)
	return NULL;
    info = &dir->i_sb->u.minix_sb;
    if (namelen > info->s_namelen)
	namelen = info->s_namelen;      /* truncate name */
    bo = 0L;
    offset = 0;
    goto minix_find;
    do {
	offset = (__u16)bo & (BLOCK_SIZE - 1);
	if (!offset) {
	    unmap_brelse(bh);
      minix_find:
	    bh = minix_bread(dir, (__u16)(bo >> BLOCK_SIZE_BITS), 0);
	    if (!bh) {
		continue;
	    }
	    map_buffer(bh);
	}
	if (minix_match(namelen, name,
		(struct minix_dir_entry *)(bh->b_data + offset), info->s_namelen)) {
	    *res_dir = (struct minix_dir_entry *) (bh->b_data + offset);
	    return bh;
	}
    } while ((bo += info->s_dirsize) < dir->i_size);
    unmap_brelse(bh);
    return NULL;
}

int minix_lookup(register struct inode *dir, const char *name, size_t len,
		 register struct inode **result)
{
    struct minix_dir_entry *de;
    struct buffer_head *bh;
    int error;

    error = -ENOENT;
    *result = NULL;

/*    if (dir) { dir != NULL always, because reached this function dereferencing dir */
	if (S_ISDIR(dir->i_mode)) {
	    debug("minix_lookup: Entering minix_find_entry\n");
	    bh = minix_find_entry(dir, name, len, &de);
	    debug("minix_lookup: minix_find_entry returned %x %d\n", bh, bh->b_mapcount);
	    if (bh) {
		*result = iget(dir->i_sb, (ino_t) de->inode);
		unmap_brelse(bh);
		error = (!*result) ? -EACCES : 0;
	    }
	}
	iput(dir);
/*    }*/
    return error;
}

/*
 *	minix_add_entry()
 *
 * adds a file entry to the specified directory, returning a possible
 * error value if it fails. If sucessful, buffer res_buf remains mapped.
 *
 */

static int minix_add_entry(register struct inode *dir, const char *name,
			   size_t namelen, ino_t ino)
{
    unsigned short offset;
    register struct buffer_head *bh;
    struct minix_dir_entry *de;
    struct minix_sb_info *info;
    __u32 bo;

    if (!dir || !dir->i_sb) return -ENOENT;
    info = &dir->i_sb->u.minix_sb;
    if (namelen > info->s_namelen)
	namelen = info->s_namelen;
    if (!namelen)
	return -ENOENT;
    bo = 0L;
    offset = 0;
    goto minix_add;
    while (1) {
	offset = (__u16)bo & (BLOCK_SIZE - 1);
	if (!offset) {
	    unmap_brelse(bh);
      minix_add:
	    bh = minix_bread(dir, (__u16)(bo >> BLOCK_SIZE_BITS), 1);
	    if (!bh) return -ENOSPC;
	    map_buffer(bh);
	}
	de = (struct minix_dir_entry *) (bh->b_data + offset);
	bo += info->s_dirsize;
	if (bo > dir->i_size) {
	    dir->i_size = bo;
	    break;
	}
	if (!de->inode)
	    break;
	if (namecompare(namelen, info->s_namelen, name, de->name)) {
	    debug("MINIXadd_entry: file %t==%s (already exists)\n",
		    name, de->name);
	    unmap_brelse(bh);
	    return -EEXIST;
	}
    }
    dir->i_mtime = dir->i_ctime = current_time();
    dir->i_dirt = 1;
    memcpy_fromfs(de->name, (char *)name, namelen);
    if (info->s_namelen > namelen)
	memset(de->name + namelen, 0, info->s_namelen - namelen);

    de->inode = (__u16)ino;
    mark_buffer_dirty(bh);
    unmap_brelse(bh);
    return 0;
}

int minix_create(register struct inode *dir, const char *name, size_t len,
		 mode_t mode, struct inode **result)
{
    register struct inode *inode = NULL;
    int error;

/*    dir != NULL always, because reached this function dereferencing dir */
    /*if (!dir) error = -ENOENT;
    else */if (!(inode = minix_new_inode(dir, mode))) error = -ENOSPC;
    else {
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
	if ((error = minix_add_entry(dir, name, len, inode->i_ino))) {
	    inode->i_nlink--;
/* if succesful, minix_new_inode() sets i_dirt to 1  */
/*	    inode->i_dirt = 1;*/
	    iput(inode);
	    inode = NULL;
	}
	iput(dir);
    }
    *result = inode;
    return error;
}

int minix_mknod(register struct inode *dir, const char *name, size_t len,
		mode_t mode, int rdev)
{
    int error;
    register struct inode *inode;
    struct buffer_head *bh;
    struct minix_dir_entry *de;

/*    if (!dir)
	return -ENOENT;*/	/* Already checked by do_mknod() */

    error = -EEXIST;
    bh = minix_find_entry(dir, name, len, &de);
    if (bh) {
	unmap_brelse(bh);
	goto mknod2;
    }
    error = -ENOSPC;
    inode = minix_new_inode(dir, mode);
    if (!inode) goto mknod2;
/*----------------------------------------------------------------------*/
    if (S_ISBLK(mode) || S_ISCHR(mode)) inode->i_rdev = to_kdev_t(rdev);
/*----------------------------------------------------------------------*/
    error = minix_add_entry(dir, name, len, inode->i_ino);
    if (error) {
	inode->i_nlink--;
/* if succesful, minix_new_inode() sets i_dirt to 1  */
/*	inode->i_dirt = 1;*/
    }
    iput(inode);
 mknod2:
    iput(dir);
    return error;
}

int minix_mkdir(register struct inode *dir, const char *name, size_t len, mode_t mode)
{
    int error;
    register struct inode *inode;
    struct buffer_head *bh;
    struct minix_dir_entry *de;

    error = -EINVAL;
/*    dir != NULL always, because reached this function dereferencing dir */
    if (/*!dir || */!dir->i_sb) goto mkdir2;
    error = -EMLINK;
    if (dir->i_nlink >= MINIX_LINK_MAX) goto mkdir2;

    error = -EEXIST;
    bh = minix_find_entry(dir, name, len, &de);
    if (bh) {
	unmap_brelse(bh);
	goto mkdir2;
    }
    error = -ENOSPC;
    inode = minix_new_inode(dir, mode);
    if (!inode) goto mkdir2;
/*--------------------------------------------------------------------------------*/
    debug("m_mkdir: new_inode succeeded\n");
    debug("m_mkdir: starting minix_bread\n");
    bh = minix_bread(inode, 0, 1);
    if (!bh) goto mkdir1;
    debug("m_mkdir: read succeeded\n");
    inode->i_size = (dir->i_sb->u.minix_sb.s_dirsize << 1);
    map_buffer(bh);
    de = (struct minix_dir_entry *) bh->b_data;
    de->inode = inode->i_ino;
    inode->i_nlink++;
    strcpy(de->name, ".");
    de = (struct minix_dir_entry *)(((char *)de) + dir->i_sb->u.minix_sb.s_dirsize);
    de->inode = dir->i_ino;
    dir->i_nlink++;
    strcpy(de->name, "..");
    mark_buffer_dirty(bh);
    unmap_brelse(bh);
    debug("m_mkdir: dir_block update succeeded\n");
/*--------------------------------------------------------------------------------*/
    error = minix_add_entry(dir, name, len, inode->i_ino);
    if (error) {
	dir->i_nlink--;
	inode->i_nlink--;
      mkdir1:
	inode->i_nlink--;
    }
/* if succesful, minix_new_inode() sets i_dirt to 1  */
/*    inode->i_dirt = 1;*/
    iput(inode);
 mkdir2:
    iput(dir);
    debug("m_mkdir: done!\n");
    return error;
}

/*
 * routine to check that the specified directory is empty (for rmdir)
 */
static int empty_dir(register struct inode *inode)
{
    unsigned short offset;
    register struct buffer_head *bh = NULL;
    struct minix_dir_entry *de;
    unsigned short dirsize;
    __u32 bo;

    if (!inode || !inode->i_sb) goto empt_dir;
    /* Cache s_dirsize to reduce dereference overhead */
    dirsize = inode->i_sb->u.minix_sb.s_dirsize;

    if ((unsigned short)(inode->i_size) & (dirsize - 1)) goto bad_dir;
    if (inode->i_size < (__u32)(dirsize << 1)) goto bad_dir;
    bh = minix_bread(inode, 0, 0);
    if (!bh) goto bad_dir;
    map_buffer(bh);
    de = (struct minix_dir_entry *) bh->b_data;
    if (!de->inode || strcmp(de->name, ".")) goto bad_dir;
    de = (struct minix_dir_entry *)(((char *)de) + dirsize);
    if (!de->inode || strcmp(de->name, "..")) goto bad_dir;
    bo = (__u32)dirsize;
    while ((bo += dirsize) < inode->i_size) {
	offset = (__u16)bo & (BLOCK_SIZE - 1);
	if (!offset) {
	    unmap_brelse(bh);
	    bh = minix_bread(inode, (__u16)(bo >> BLOCK_SIZE_BITS), 0);
	    if (!bh)
		continue;
	    map_buffer(bh);
	}
	de = (struct minix_dir_entry *) (bh->b_data + offset);
	if (de->inode) {
	    unmap_brelse(bh);
	    return 0;
	}
    }
    brelse(bh);
    goto empt_dir;
  bad_dir:
    unmap_brelse(bh);
    printk("Bad directory on device %D\n", inode->i_dev);
  empt_dir:
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
    if (bh) {
	retval = -EPERM;
	if ((inode = iget(dir->i_sb, (ino_t) de->inode))) {
	    if (((dir->i_mode & S_ISVTX) && !suser() &&
		    (current->euid != inode->i_uid) && (current->euid != dir->i_uid))
		|| (inode->i_dev != dir->i_dev)
		|| (inode == dir)	/* we may not delete ".", but "../dir" is ok */
		)
		;
	    else if (!S_ISDIR(inode->i_mode)) {
		retval = -ENOTDIR;
	    }
	    else if (!empty_dir(inode)) {
		retval = -ENOTEMPTY;
	    }
	    else if (de->inode != inode->i_ino) {
		retval = -ENOENT;
	    }
	    else if (inode->i_count > 1) {
		retval = -EBUSY;
	    }
	    else {
		if (inode->i_nlink != 2)
		    printk("empty directory has nlink!=2 (%u)\n", inode->i_nlink);
		de->inode = 0;

		mark_buffer_dirty(bh);
		inode->i_nlink = 0;
		inode->i_dirt = 1;
		inode->i_ctime = dir->i_ctime = dir->i_mtime = current_time();
		dir->i_nlink--;
		dir->i_dirt = 1;
		retval = 0;
	    }
	    iput(inode);
	}
	unmap_brelse(bh);
    }
    iput(dir);
    return retval;
}

int minix_unlink(register struct inode *dir, char *name, size_t len)
{
    int retval;
    register struct inode *inode;
    struct buffer_head *bh;
    struct minix_dir_entry *de;

    goto init_loop;
    do {
	iput(inode);
	unmap_brelse(bh);
	schedule();
  init_loop:
	retval = -ENOENT;
	inode = NULL;
	bh = minix_find_entry(dir, name, len, &de);
	if (!bh) goto end_unlink2;
	if (!(inode = iget(dir->i_sb, (ino_t) de->inode))) goto end_unlink1;
	retval = -EPERM;
	if (S_ISDIR(inode->i_mode)) goto end_unlink;
    } while (de->inode != inode->i_ino);
    if ((dir->i_mode & S_ISVTX) && !suser() &&
	current->euid != inode->i_uid && current->euid != dir->i_uid)
	goto end_unlink;
    if (!inode->i_nlink) {
	printk("Deleting nonexistent file dev %D %lu, %u\n",
	       inode->i_dev, inode->i_ino, inode->i_nlink);
	inode->i_nlink = 1;
    }
    de->inode = 0;

    mark_buffer_dirty(bh);
    dir->i_ctime = dir->i_mtime = current_time();
    dir->i_dirt = 1;
    inode->i_nlink--;
    inode->i_ctime = dir->i_ctime;
    inode->i_dirt = 1;
    retval = 0;

  end_unlink:
    iput(inode);
  end_unlink1:
    unmap_brelse(bh);
  end_unlink2:
    iput(dir);
    return retval;
}

int minix_symlink(struct inode *dir, char *name, size_t len, char *symname)
{
    int error;
    register struct inode *inode;
    register struct buffer_head *bh;
    struct minix_dir_entry *de;

    error = -EEXIST;
    bh = minix_find_entry(dir, name, len, &de);
    if (bh) {
	unmap_brelse(bh);
	goto symlink2;
    }
    error = -ENOSPC;
    inode = minix_new_inode(dir, S_IFLNK);
    if (!inode) goto symlink2;
/*----------------------------------------------------------------------*/
    bh = minix_bread(inode, 0, 1);
    if (!bh) goto symlink1;
    if ((error = strlen_fromfs(symname, 1023)) > 1023) error = 1023;
    inode->i_size = (__u32) error;
    map_buffer(bh);
    memcpy_fromfs(bh->b_data, symname, error);
    bh->b_data[error] = 0;
    mark_buffer_dirty(bh);
    unmap_brelse(bh);
/*----------------------------------------------------------------------*/
    error = minix_add_entry(dir, name, len, inode->i_ino);
    if (error) {
      symlink1:
	inode->i_nlink--;
/* if succesful, minix_new_inode() sets i_dirt to 1  */
/*	inode->i_dirt = 1;*/
    }
    iput(inode);
 symlink2:
    iput(dir);
    return error;
}

int minix_link(register struct inode *dir, char *name, size_t len,
		register struct inode *oldinode)
{
    int error;
    struct buffer_head *bh;
    struct minix_dir_entry *de;

    if (S_ISDIR(oldinode->i_mode)) error = -EPERM;
    else if (oldinode->i_nlink >= MINIX_LINK_MAX) error = -EMLINK;
    else if ((bh = minix_find_entry(dir, name, len, &de))) {
	unmap_brelse(bh);
	error = -EEXIST;
    }
    else if (!(error = minix_add_entry(dir, name, len, oldinode->i_ino))) {
	oldinode->i_nlink++;
	oldinode->i_ctime = current_time();
	oldinode->i_dirt = 1;
    }
    iput(dir);
    iput(oldinode);
    return error;
}
