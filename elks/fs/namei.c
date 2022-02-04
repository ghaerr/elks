/*
 *  linux/fs/namei.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/*
 * Some corrections by tytso.
 */

#include <arch/segment.h>
#include <arch/system.h>

#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/string.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/stat.h>
#include <linuxmt/mm.h>
#include <linuxmt/debug.h>

#define ACC_MODE(x) ("\000\004\002\006"[(x)&O_ACCMODE])

/*
 *	permission()
 *
 * is used to check for read/write/execute permissions on a file.
 * We use "fsuid" for this, letting us set arbitrary permissions
 * for filesystem access without changing the "normal" uids which
 * are used for other things..
 */

int permission(register struct inode *inode, int mask)
{
    __u16 mode = inode->i_mode;
    int error = -EACCES;

    if (mask & MAY_WRITE) {	/* disallow writing over running programs */
	__ptask p = &task[0];
	do {
	    if (p->state <= TASK_STOPPED && (p->t_inode == inode))
		return -EBUSY;
	} while (++p < &task[MAX_TASKS]);
    }
    if ((mask & MAY_WRITE) && IS_RDONLY(inode) &&
        !S_ISCHR(inode->i_mode) && !S_ISBLK(inode->i_mode)) /* allow writable devices*/
	error = -EROFS;

#ifdef BLOAT_FS
    else if (inode->i_op && inode->i_op->permission)
	error = inode->i_op->permission(inode, mask);
#endif

#ifdef HAVE_IMMUTABLE
    else if ((mask & S_IWOTH) && IS_IMMUTABLE(inode))
	/* Nobody gets write access to an immutable file */
	;
#endif

    else {
	if (current->euid == inode->i_uid)
	    mode >>= 6;
	else if (in_group_p(inode->i_gid))
	    mode >>= 3;
	if (((mode & mask & 0007) == (__u16) mask) || suser())
	    error = 0;
    }
    return error;
}

/*
 * get_write_access() gets write permission for a file.
 * put_write_access() releases this write permission.
 */

#ifdef BLOAT_FS

int get_write_access(struct inode *inode)
{
    inode->i_wcount++;
    return 0;
}

void put_write_access(struct inode *inode)
{
    inode->i_wcount--;
}

#endif

/*
 * lookup() looks up one part of a pathname, using the fs-dependent
 * routines for it. It also checks for fathers (pseudo-roots, mount-points)
 */

static int lookup(register struct inode *dir, char *name, size_t len,
	   struct inode **result)
{
    register struct inode_operations *iop;
    int perm, retval = -ENOENT;

    *result = NULL;
    if (dir) {
	dir->i_count++;				/* lookup eats the dir */
	/* check permissions before traversing mount-points */
	perm = permission(dir, MAY_EXEC);
	if ((len == 2) && !fs_memcmp(name, "..", 2)) {
	    if (dir == current->fs.root) {
		*result = dir;
		retval = 0;
		goto lkp_end;
	    } else if ((dir->i_sb) && (dir == dir->i_sb->s_mounted)) {
		iput(dir);
		dir = dir->i_sb->s_covered;
		if (!dir) {
		    goto lkp_end;
		}
		dir->i_count++;
	    }
	}
	iop = dir->i_op;
	if (!iop || !iop->lookup) {
	    printk("Oops - trying to access dir\n");	//FIXME remove
	    iput(dir);
	    retval = -ENOTDIR;
	} else if (perm != 0) {
	    iput(dir);
	    retval = perm;
	} else if (!len) {
	    *result = dir;
	    retval = 0;
	} else
	    retval = iop->lookup(dir, name, len, result);
    }

  lkp_end:
    return retval;
}

static int follow_link(struct inode *dir, register struct inode *inode,
		int flag, int mode, struct inode **res_inode)
{
    struct inode_operations *iop;
    int error = 0;

    *res_inode = inode;
    if (!dir || !inode) {
	iput(inode);
	*res_inode = NULL;
	error = -ENOENT;
    } else if ((iop = inode->i_op) && iop->follow_link) {
	dir->i_count++;
	error = iop->follow_link(dir, inode, flag, mode, res_inode);
    }
    iput(dir);
    return error;
}

/*
 *	dir_namei()
 *
 * dir_namei() returns the inode of the directory of the
 * specified name, and the name within that directory.
 */

static int dir_namei(register char *pathname, size_t * namelen,
		     char **name, register struct inode *base, struct inode **res_inode)
{
    char *thisname;
    struct inode *inode;
    struct inode *baser;
    size_t len;
    int error = 0;
    unsigned char c;

    debug("namei: entered dir_namei\n");
    *res_inode = NULL;
    if (!base) {
	base = current->fs.pwd;
	base->i_count++;
    }
    if (get_user_char(pathname) == '/') {
	iput(base);
	base = current->fs.root;
	pathname++;
	base->i_count++;
    }
    do {
	thisname = pathname;
	while ((c = get_user_char(pathname++)) && (c != '/'))
	    /* Do nothing */ ;
	len = pathname - thisname - 1;
	if (!c) {
	    if (!base->i_op || !base->i_op->lookup) {
		iput(base);
		error = -ENOTDIR;
	    } else {
		*name = thisname;
		*namelen = len;
		*res_inode = base;
	    }
	    debug("namei: left dir_namei succesfully\n");
	    break;
	} else { /* discard multiple '/'s*/
	    while (get_user_char(pathname) == '/')
		pathname++;
	}
	error = lookup(base, thisname, len, &inode);
	if (error)
	    iput(base);
	else {
	    error = follow_link(base, inode, 0, 0, &baser);
	    base = baser;
	}
    } while (!error);

    return error;
}

/*
 *	_namei()
 *
 * Get the inode of 'pathname'.
 *
 * follow_links != 0 means can follow links
 *
 */
int _namei(char *pathname, register struct inode *base, int follow_links,
	   struct inode **res_inode)
{
    char *basename;
    int error;
    size_t namelen;
    struct inode *inode;

    debug("_namei: calling dir_namei\n");
    *res_inode = NULL;
    error = dir_namei(pathname, &namelen, &basename, base, &inode);
    debug("_namei: dir_namei returned %d\n", error);
    if (!error) {
	base = inode;
	debug("_namei: calling lookup\n");
	error = lookup(base, basename, namelen, &inode);
	debug("_namei: lookup returned %d\n", error);
	if (!error) {
	    if (follow_links) {
		error = follow_link(base, inode, 0, 0, &inode);
		if (error) goto namei_end;
	    } else
		iput(base);
	    *res_inode = inode;
	} else
	    iput(base);
    }

  namei_end:
    return error;
}

/*
 *	namei()
 *
 * is used by most simple commands to get the inode of a specified name.
 * Open, link etc use their own routines, but this is enough for things
 * like 'chmod' etc.
 *
 * dir can be: IS_DIR, pathname must be a directory
 *             NOT_DIR, pathname must not be a directory
 *             0,       pathname may be any
 */
int namei(char *pathname, register struct inode **res_inode, int dir, int perm)
{
    register struct inode *inode;
    int error = _namei(pathname, NULL, 1, res_inode);

    if (!error) {
	inode = *res_inode;
	if (dir == NOT_DIR && S_ISDIR(inode->i_mode)) error = -EISDIR;
	else if (dir == IS_DIR && !S_ISDIR(inode->i_mode)) error = -ENOTDIR;
	else if (perm) error = permission(inode, perm);
	if (error) iput(inode);
    }
    return error;
}

/*
 *	open_namei()
 *
 * namei for open - this is in fact almost the whole open-routine.
 *
 * Note that the low bits of "flag" aren't the same as in the open
 * system call - they are 00 - no permissions needed
 *			  01 - read permission needed
 *			  10 - write permission needed
 *			  11 - read/write permissions needed
 * which is a lot more logical, and also allows the "no perm" needed
 * for symlinks (where the permissions are checked later).
 */

int open_namei(char *pathname, int flag, int mode,
	       struct inode **res_inode, struct inode *base)
{
    register struct inode *dirp;
    register struct inode_operations *iop;
    struct inode *dir;
    char *basename;
    size_t namelen;
    int error;
    struct inode *inode;

    debug("NAMEI: open namei entered\n");
    mode = S_IFREG | (mode & S_IALLUGO);
    error = dir_namei(pathname, &namelen, &basename, base, &dir);
    if (error) goto onamei_end;

    dirp = dir;
    if (!namelen) {		/* special case: '/usr/' etc */
	if (flag & 2) {
	    iput(dirp);
	    error = -EISDIR;
	} else if ((error = permission(dirp, ACC_MODE(flag))) != 0) {
	    /* thanks to Paul Pluzhnikov for noticing this was missing.. */
	    iput(dirp);
	} else *res_inode = dirp;
	goto onamei_end;
    }
    if (flag & O_CREAT) {
	down(&dirp->i_sem);
	error = lookup(dirp, basename, namelen, &inode);
	if (!error) {
	    if (flag & O_EXCL) {
		iput(inode);
		error = -EEXIST;
	    }
	}
	else if (!(error = permission(dirp, MAY_WRITE | MAY_EXEC))) {
	    iop = dirp->i_op;
	    if (!iop || !iop->create)
		error = -EACCES;
	    else {
		dirp->i_count++;	/* create eats the dir */
		error = iop->create(dirp, basename, namelen, mode, res_inode);
		up(&dirp->i_sem);
		iput(dirp);
		goto onamei_end;
	    }
	}
	up(&dirp->i_sem);
    } else error = lookup(dirp, basename, namelen, &inode);
    if (error) {
	iput(dirp);
	goto onamei_end;
    }
    error = follow_link(dirp, inode, flag, mode, &inode);
    if (error) goto onamei_end;
    dirp = inode;		/* dirp not used anymore */
#define inode dirp
    if (S_ISDIR(inode->i_mode) && (flag & 2)) {
	error = -EISDIR;
    } else if ((error = permission(inode, ACC_MODE(flag))) == 0) {
	if (S_ISBLK(inode->i_mode) || S_ISCHR(inode->i_mode)) {
	    if (IS_NODEV(inode)) error = -EACCES;
	    else flag &= ~O_TRUNC;
	}
    }
    if (!error) {
	if (flag & O_TRUNC) {

#ifdef USE_NOTIFY_CHANGE
	    struct iattr newattrs;
#endif

#ifndef get_write_access
	    if ((error = get_write_access(inode))) {
		iput(inode);
		return error;
	    }
#endif

#ifdef USE_NOTIFY_CHANGE
	    newattrs.ia_size = 0;
	    newattrs.ia_valid = ATTR_SIZE;
	    if ((error = notify_change(inode, &newattrs))) {
		put_write_access(inode);
		iput(inode);
		return error;
	    }
#else
	    inode->i_size = 0L;
#endif

	    down(&inode->i_sem);
	    if ((iop = inode->i_op) && iop->truncate) iop->truncate(inode);
	    up(&inode->i_sem);
	    inode->i_dirt = 1;
	    put_write_access(inode);
	}
	*res_inode = inode;
    } else iput(inode);
#undef inode

  onamei_end:
    return error;
}

int do_mknod(char *pathname, int offst, int mode, dev_t dev)
{
#ifdef CONFIG_FS_RO
    return -EROFS;
#else
    register struct inode *dirp;
    register struct inode_operations *iop;
    struct inode *dir;
    char *basename;
    size_t namelen;
    /* declare function pointer with varying arguments as specified by ()
     * a pair of parentheses to bind the * to the name instead of the return type */
    int (*op) ();
    int error = dir_namei(pathname, &namelen, &basename, NULL, &dir);

    if (!error) {
	dirp = dir;
	if (!namelen) {
	    if (offst == (int)offsetof(struct inode_operations,link))
		error = -EPERM;
	    else
		error = -ENOENT;
	}
/*	else if (IS_RDONLY(dirp)) error = -EROFS; */
	else if ((offst == (int)offsetof(struct inode_operations,link))
		    && (dirp->i_dev != ((struct inode *)mode)->i_dev)) error = -EXDEV;
	else if (!(error = permission(dirp, MAY_WRITE | MAY_EXEC))) {
	    iop = dirp->i_op;
	    if (!iop || !(op = (*(int (**)())((char *)iop + offst))))
		error = -EPERM;
	    else {
		dirp->i_count++;
		down(&dirp->i_sem);
		error = (offst != (int)offsetof(struct inode_operations,mknod)
			? op(dirp, basename, namelen, mode)
			: op(dirp, basename, namelen, mode, dev)
		    );
		up(&dirp->i_sem);
	    }
	}
	iput(dirp);
    }
    return error;
#endif
}

int sys_mknod(char *pathname, int mode, dev_t dev)
{
#ifdef CONFIG_FS_RO
    return -EROFS;
#else
    if (S_ISDIR(mode) || (!S_ISFIFO(mode) && !suser())) return -EPERM;

    switch (mode & S_IFMT) {
    case 0:
	mode |= S_IFREG;
    case S_IFREG:
    case S_IFCHR:
    case S_IFBLK:
    case S_IFIFO:
    case S_IFSOCK:
	break;
    default:
	return -EINVAL;
    }

    return do_mknod(pathname, offsetof(struct inode_operations,mknod), mode, dev);
#endif
}

int sys_mkdir(char *pathname, int mode)
{
#ifdef CONFIG_FS_RO
    return -EROFS;
#else
    return do_mknod(pathname, offsetof(struct inode_operations,mkdir), (mode & 0777)|S_IFDIR, 0);
#endif
}

int do_rmthing(char *pathname, size_t offst)
{
    register struct inode *dirp;
    register struct inode_operations *iop;
    struct inode *dir;
    char *basename;
    size_t namelen;
    int (*op) ();
    int error;

    error = dir_namei(pathname, &namelen, &basename, NULL, &dir);
    if (!error) {
	dirp = dir;
	if (!namelen)
	    error = -ENOENT; /*(offst == offsetof(struct inode_operations,unlink)
				    ? -EPERM : -ENOENT);*/
/*	else if (IS_RDONLY(dirp))
	    error = -EROFS;*/
	else if (!(error = permission(dirp, MAY_WRITE | MAY_EXEC))) {
	    iop = dirp->i_op;
	    if (!iop || !(op = (*(int (**)())((char *)iop + offst)))) {
		error = -EPERM;
	    } else {
		dirp->i_count++;
/*		down(&dirp->i_sem);*/
		error = op(dirp, basename, namelen);
/*		up(&dirp->i_sem);*/
	    }
	}
	iput(dirp);
    }
    return error;
}

int sys_rmdir(char *pathname)
{
    return do_rmthing(pathname, offsetof(struct inode_operations,rmdir));
}

int sys_unlink(char *pathname)
{
    debug_file("UNLINK '%t'\n", pathname);
    return do_rmthing(pathname, offsetof(struct inode_operations,unlink));
}

int sys_symlink(char *oldname, char *pathname)
{
#ifdef CONFIG_FS_RO
    return -EROFS;
#else
    return do_mknod(pathname, offsetof(struct inode_operations,symlink), (int)oldname, 0);
#endif
}

int sys_link(char *oldname, char *pathname)
{
#ifdef CONFIG_FS_RO
    return -EROFS;
#else
    struct inode *oldinode;
    int error;

    debug_file("LINK '%t' '%t'\n", oldname, pathname);
    error = namei(oldname, &oldinode, 0, 0);
    if (!error)
	error = do_mknod(pathname, offsetof(struct inode_operations,link), (int)oldinode, 0);
    return error;
#endif
}

/*  This probably isn't a proper implementation of sys_rename, but we
 *  don't have space for one, and I don't want to write one when I can
 *  make a simple 6-line version like this one :) - Chad
 */

int sys_rename(register char *oldname, char *pathname)
{
    int err;

    return !(err = sys_link(oldname, pathname)) ? sys_unlink(oldname) : err;

}
