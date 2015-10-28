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
 * If succesful, the buffer cache returns MAPPED, otherwise returns NULL
 *
 */

static struct buffer_head *minix_find_entry(register struct inode *dir,
					    char *name, size_t namelen,
					    struct minix_dir_entry **res_dir)
{
    register struct buffer_head *bh;
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
	    map_buffer(bh);
	}
	*res_dir = (struct minix_dir_entry *) (bh->b_data + offset);

	if (minix_match(namelen, name, bh, &offset, info)) {
	    return bh;
	}
	if (offset >= BLOCK_SIZE) {
	    unmap_brelse(bh);
	    bh = NULL;
	    offset = 0;
	    block++;
	}
    }
    unmap_brelse(bh);
    *res_dir = NULL;
    return NULL;
}

int minix_lookup(register struct inode *dir, char *name, size_t len,
		 register struct inode **result)
{
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
		unmap_brelse(bh);
		*result = iget(dir->i_sb, (ino_t) de->inode);
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
 * error value if it fails. If sucessful, buffer res_buf remains mapped.
 *
 */

static int minix_add_entry(register struct inode *dir,
			   char *name,
			   size_t namelen,
			   ino_t ino)
{
    unsigned short block;
    loff_t offset;
    register struct buffer_head *bh;
    struct minix_dir_entry *de;
    struct minix_sb_info *info;

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
	    memcpy_fromfs(de->name, name, namelen);
	    if((i = info->s_namelen - namelen) > 0)
		memset(de->name + namelen, 0, i);

#ifdef BLOAT_FS
	    dir->i_version = ++event;
#endif

	    de->inode = ino;
	    mark_buffer_dirty(bh, 1);
	    unmap_brelse(bh);
	    break;
	}
	if (offset >= BLOCK_SIZE) {
	    unmap_brelse(bh);
	    bh = NULL;
	    offset = 0;
	    block++;
	}
    }
    return 0;
}

int minix_create(register struct inode *dir, char *name, size_t len,
		 int mode, struct inode **result)
{
    register struct inode *inode = NULL;
    int error;

    error = -ENOENT;
    if (!dir)
	goto create2;

    error = -ENOSPC;
    inode = minix_new_inode(dir, (__u16)mode);
    if (!inode)
	goto create1;
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
    error = minix_add_entry(dir, name, len, inode->i_ino);
    if(error) {
	inode->i_nlink--;
	inode->i_dirt = 1;
	iput(inode);
	inode = NULL;
    }
 create1:
    iput(dir);
 create2:
    *result = inode;
    return error;
}

int minix_mknod(register struct inode *dir, char *name, size_t len,
		int mode, int rdev)
{
    int error;
    register struct inode *inode;
    struct buffer_head *bh;
    struct minix_dir_entry *de;

    error = -ENOENT;
    if (!dir)
	goto mknod2;

    error = -EEXIST;
    bh = minix_find_entry(dir, name, len, &de);
    if (bh) {
	unmap_brelse(bh);
	goto mknod1;
    }
    error = -ENOSPC;
    inode = minix_new_inode(dir, (__u16)mode);
    if (!inode)
	goto mknod1;
/*----------------------------------------------------------------------*/
    if (S_ISBLK(mode) || S_ISCHR(mode))
	inode->i_rdev = to_kdev_t(rdev);
/*----------------------------------------------------------------------*/
    error = minix_add_entry(dir, name, len, inode->i_ino);
    if(error) {
	inode->i_nlink--;
	inode->i_dirt = 1;
    }
    iput(inode);
 mknod1:
    iput(dir);
 mknod2:
    return error;
}

int minix_mkdir(register struct inode *dir, char *name, size_t len, int mode)
{
    int error;
    register struct inode *inode;
    struct buffer_head *bh;
    struct minix_dir_entry *de;

    error = -EINVAL;
    if (!dir || !dir->i_sb)
	goto mkdir2;
    error = -EMLINK;
    if (dir->i_nlink >= MINIX_LINK_MAX)
	goto mkdir2;

    error = -EEXIST;
    bh = minix_find_entry(dir, name, len, &de);
    if (bh) {
	unmap_brelse(bh);
	goto mkdir2;
    }
    error = -ENOSPC;
    inode = minix_new_inode(dir, (__u16)mode);
    if (!inode)
	goto mkdir2;
/*--------------------------------------------------------------------------------*/
    debug("m_mkdir: new_inode succeeded\n");
    debug("m_mkdir: starting minix_bread\n");
    bh = minix_bread(inode, 0, 1);
    if (!bh) {
	goto mkdir1;
    }
    debug("m_mkdir: read succeeded\n");
    inode->i_size = (dir->i_sb->u.minix_sb.s_dirsize << 1);
    map_buffer(bh);
    de = (struct minix_dir_entry *) bh->b_data;
    de->inode = inode->i_ino;
    strcpy(de->name, ".");
    de = (struct minix_dir_entry *)(((char *)de) + dir->i_sb->u.minix_sb.s_dirsize);
    de->inode = dir->i_ino;
    strcpy(de->name, "..");
    mark_buffer_dirty(bh, 1);
    unmap_brelse(bh);
    debug("m_mkdir: dir_block update succeeded\n");
/*--------------------------------------------------------------------------------*/
    error = minix_add_entry(dir, name, len, inode->i_ino);
    if (error) {
      mkdir1:
	inode->i_nlink--;
    }
    else {
	inode->i_nlink++;
	dir->i_nlink++;
    }
    inode->i_dirt = 1;
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
    unsigned short block;
    loff_t offset;
    register struct buffer_head *bh;
    struct minix_dir_entry *de;

    if (!inode || !inode->i_sb)
	goto empt_dir;
    block = 0;
    bh = NULL;
    offset = (inode->i_sb->u.minix_sb.s_dirsize << 1);
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
    goto empt_dir;
  bad_dir:
    unmap_brelse(bh);
    printk("Bad directory on device %s\n", kdevname(inode->i_dev));
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
    if (!bh)
	goto end_rmdir;
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
	if (!bh)
	    goto end_unlink;
	if (!(inode = iget(dir->i_sb, (ino_t) de->inode)))
	    goto end_unlink;
	retval = -EPERM;
	if (S_ISDIR(inode->i_mode))
	    goto end_unlink;
    } while(de->inode != inode->i_ino);
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
    if (!inode)
	goto symlink2;
/*----------------------------------------------------------------------*/
    bh = minix_bread(inode, 0, 1);
    if (!bh)
	goto symlink1;
    map_buffer(bh);
    if((error = strlen_fromfs(symname)) > 1023)
	error = 1023;
    memcpy_fromfs(bh->b_data, symname, error);
    bh->b_data[error] = 0;
    inode->i_size = (__u32) error;
    mark_buffer_dirty(bh, 1);
    unmap_brelse(bh);
/*----------------------------------------------------------------------*/
    error = minix_add_entry(dir, name, len, inode->i_ino);
    if (error) {
      symlink1:
	inode->i_nlink--;
	inode->i_dirt = 1;
    }
    iput(inode);
 symlink2:
    iput(dir);
    return error;
}

int minix_link(register struct inode *oldinode, register struct inode *dir,
		char *name, size_t len)
{
    int error;
    struct buffer_head *bh;
    struct minix_dir_entry *de;

    error = -EPERM;
    if (S_ISDIR(oldinode->i_mode))
	goto mlink_err;
    error = -EMLINK;
    if (oldinode->i_nlink >= MINIX_LINK_MAX)
	goto mlink_err;
    error = -EEXIST;
    bh = minix_find_entry(dir, name, len, &de);
    if (bh) {
	unmap_brelse(bh);
	goto mlink_err;
    }
    error = minix_add_entry(dir, name, len, oldinode->i_ino);
    if (!error) {
	oldinode->i_nlink++;
	oldinode->i_ctime = CURRENT_TIME;
	oldinode->i_dirt = 1;
    }
 mlink_err:
    iput(oldinode);
    iput(dir);
    return error;
}
