/*
 *  linux/fs/namei.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/*
 * Some corrections by tytso.
 */

#include <arch/segment.h>
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

int permission(inode,mask)
register struct inode * inode;
int mask;
{
	int mode = inode->i_mode;

#ifdef BLOAT_FS
	if (inode->i_op && inode->i_op->permission)
		return inode->i_op->permission(inode, mask);
	else if ((mask & S_IWOTH) && IS_IMMUTABLE(inode))
#else
	if ((mask & S_IWOTH) && IS_IMMUTABLE(inode))
#endif
		return -EACCES; /* Nobody gets write access to an immutable file */
	else if (current->euid == inode->i_uid)
		mode >>= 6;
	else if (in_group_p(inode->i_gid))
		mode >>= 3;
	if (((mode & mask & 0007) == mask) || suser())
		return 0;
	return -EACCES;
}

/*
 * get_write_access() gets write permission for a file.
 * put_write_access() releases this write permission.
 */

#ifdef BLOAT_FS 
int get_write_access(inode)
struct inode *inode;
{
/*	struct task_struct * p; */
	inode->i_wcount++;
	return 0;
}

void put_write_access(inode)
struct inode * inode;
{
	inode->i_wcount--;
}
#endif

/*
 * lookup() looks up one part of a pathname, using the fs-dependent
 * routines (currently minix_lookup) for it. It also checks for
 * fathers (pseudo-roots, mount-points)
 */

int lookup(dir,name,len,result)
register struct inode * dir;
char * name;
size_t len;
struct inode ** result;
{
	register struct inode_operations * iop;
	int perm, retval;

	*result = NULL;
	if (!dir) {
		return -ENOENT;
	}
/* check permissions before traversing mount-points */
	perm = permission(dir,MAY_EXEC);
	if (len==2 && get_fs_byte(name) == '.' && get_fs_byte(name+1) == '.') {
		if (dir == current->fs.root) {
			*result = dir;
			return 0;
		} else if ((dir->i_sb) && (dir == dir->i_sb->s_mounted)) {
			iput(dir);
			dir = dir->i_sb->s_covered;
			if (!dir) {
				return -ENOENT;
			}
			dir->i_count++;
		}
	}
	iop = dir->i_op;
	if (!iop || !iop->lookup) {
#if 1
		panic("Oops - trying to access dir\n"); 
#endif
		iput(dir);
		return -ENOTDIR;
	}
 	if (perm != 0) {
		iput(dir);
		return perm;
	}
	if (!len) {
		*result = dir;
		return 0;
	}
	printd_namei("lookup: calling fs lookup\n");
	retval = iop->lookup(dir,name,len,result);
	printd_namei1("lookup: returning %d\n", retval);
	return retval;
}

int follow_link(dir,inode,flag,mode,res_inode)
struct inode * dir;
register struct inode * inode;
int flag;
int mode;
struct inode ** res_inode;
{
	register struct inode_operations * iop = inode->i_op;
	if (!dir || !inode) {
		iput(dir);
		iput(inode);
		*res_inode = NULL;
		return -ENOENT;
	}
	if (!iop || !iop->follow_link) {
		iput(dir);
		*res_inode = inode;
		return 0;
	}
	return iop->follow_link(dir,inode,flag,mode,res_inode);
}

/*
 *	dir_namei()
 *
 * dir_namei() returns the inode of the directory of the
 * specified name, and the name within that directory.
 */
 
static int dir_namei(pathname,namelen,name,base,res_inode)
register char * pathname;
size_t * namelen;
char ** name;
struct inode * base;
struct inode ** res_inode;
{
	char c;
	register char * thisname;
	int error;
	size_t len;
	struct inode * inode;
#if 0
	printk("namei: entered dir_namei\n");
#endif 

	*res_inode = NULL;
	if (!base) {
		base = current->fs.pwd;
		base->i_count++;
	}
	if ((c = get_fs_byte(pathname)) == '/') {
		iput(base);
		base = current->fs.root;
		pathname++;
		base->i_count++;
	}
	while (1) {
		thisname = pathname;
		for(len=0;(c = get_fs_byte(pathname++))&&(c != '/');len++)
			/* nothing */ ;
		if (!c)
			break;
		base->i_count++;
		error = lookup(base,thisname,len,&inode);
		if (error) {
			iput(base);
			return error;
		}
		error = follow_link(base,inode,0,0,&base);
		if (error)
			return error;
	}
	if (!base->i_op || !base->i_op->lookup) {
		iput(base);
		return -ENOTDIR;
	}
	*name = thisname;
	*namelen = len;
	*res_inode = base;
#if 0	
	printk("namei: left dir_namei succesfully\n");
#endif
	return 0;
}

static int _namei(pathname,base,follow_links,res_inode)
char * pathname;
struct inode * base;
int follow_links;
register struct inode ** res_inode;
{
	char * basename;
	int error;
	size_t namelen;
	struct inode * inode;

	printd_namei("_namei: calling dir_namei\n");
	*res_inode = NULL;
	error = dir_namei(pathname,&namelen,&basename,base,&base);
	printd_namei1("_namei: dir_namei returned %d\n", error);
	if (error)
		return error;
	base->i_count++;	/* lookup uses up base */
	printd_namei("_namei: calling lookup\n");
	error = lookup(base,basename,namelen,&inode);
	printd_namei1("_namei: lookup returned %d\n", error);
	if (error) {
		iput(base);
		return error;
	}
	if (follow_links) {
		error = follow_link(base,inode,0,0,&inode);
		if (error)
			return error;
	} else
		iput(base);
	*res_inode = inode;
	return 0;
}

int lnamei(pathname, res_inode)
char *pathname;
struct inode ** res_inode;
{
	int error;
	printd_namei("lnamei: entering namei.\n");
	error = _namei(pathname,NULL,0,res_inode);
	printd_namei1("lnamei: returning %d\n", error);
	return error;
}

/*
 *	namei()
 *
 * is used by most simple commands to get the inode of a specified name.
 * Open, link etc use their own routines, but this is enough for things
 * like 'chmod' etc.
 */
int namei(pathname,res_inode, dir, perm)
char * pathname;
register struct inode ** res_inode;
int dir;
int perm;
{
	int error;
	register struct inode *inode;
	error = _namei(pathname,NULL,1,res_inode);
	if(error)
		return error;
	inode=*res_inode;
	if(dir==NOT_DIR && S_ISDIR(inode->i_mode))
	{
		iput(inode);
		return -EISDIR;
	}
	if(dir==IS_DIR && !S_ISDIR(inode->i_mode))
	{
		iput(inode);
		return -ENOTDIR;
	}
	if(perm && (error=permission(inode, perm))!=0)
	{
		iput(inode);
		return error;
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
 
int open_namei(pathname,flag,mode,res_inode,base)
char * pathname;
int flag;
int mode;
register struct inode ** res_inode;
struct inode * base;
{
	char * basename;
	size_t namelen;
	int error;
	struct inode * dir;
	struct inode * inode;
	register struct inode_operations * iop;
#if 0
	printk("NAMEI: open namei entered\n");
#endif

	mode &= S_IALLUGO & ~current->fs.umask;
	mode |= S_IFREG;
	error = dir_namei(pathname,&namelen,&basename,base,&dir);
	if (error)
		return error;
	if (!namelen) {			/* special case: '/usr/' etc */
		if (flag & 2) {
			iput(dir);
			return -EISDIR;
		}
		/* thanks to Paul Pluzhnikov for noticing this was missing.. */
		if ((error = permission(dir,ACC_MODE(flag))) != 0) {
			iput(dir);
			return error;
		}
		*res_inode=dir;
		return 0;
	}
	dir->i_count++;		/* lookup eats the dir */
	if (flag & O_CREAT) {
#ifdef NOT_YET
		down(&dir->i_sem);
#endif		
		error = lookup(dir,basename,namelen,&inode);
		iop = dir->i_op;
		if (!error) {
			if (flag & O_EXCL) {
				iput(inode);
				error = -EEXIST;
			}
		} else if ((error = permission(dir,MAY_WRITE | MAY_EXEC)) != 0)
			;	/* error is already set! */
		else if (!iop || !iop->create)
			error = -EACCES;
		else if (IS_RDONLY(dir))
			error = -EROFS;
		else {
			dir->i_count++;		/* create eats the dir */
			error = iop->create(dir,basename,namelen,mode,res_inode);
#ifdef NOT_YET			
			up(&dir->i_sem);
#endif			
			iput(dir);
			return error;
		}
#ifdef NOT_YET		
		up(&dir->i_sem);
#endif		
	} else
		error = lookup(dir,basename,namelen,&inode);
	if (error) {
		iput(dir);
		return error;
	}
	error = follow_link(dir,inode,flag,mode,&inode);
	if (error)
		return error;
	if (S_ISDIR(inode->i_mode) && (flag & 2)) {
		iput(inode);
		return -EISDIR;
	}
	if ((error = permission(inode,ACC_MODE(flag))) != 0) {
		iput(inode);
		return error;
	}
	if (S_ISBLK(inode->i_mode) || S_ISCHR(inode->i_mode)) {
		if (IS_NODEV(inode)) {
			iput(inode);
			return -EACCES;
		}
		flag &= ~O_TRUNC;
	} else {
		if (IS_RDONLY(inode) && (flag & 2)) {
			iput(inode);
			return -EROFS;
		}
	}
	if (flag & O_TRUNC) {
		struct iattr newattrs;

#ifdef BLOAT_FS
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
#ifdef NOT_YET		
		down(&inode->i_sem);
#endif		
		inode->i_size = 0;
		if ((iop = inode->i_op) && iop->truncate) {
			iop->truncate(inode);
		}
#ifdef NOT_YET			
		up(&inode->i_sem);
#endif		
		inode->i_dirt = 1;
		put_write_access(inode);
	}
	*res_inode = inode;
#if 0	
	printk("NAMEI: open namei left\n");
#endif
	return 0;
}

#ifndef CONFIG_FS_RO
int do_mknod(filename,mode,dev)
char * filename;
int mode;
dev_t dev;
{
	char * basename;
	size_t namelen;
	int error;
	struct inode * dir;
	register struct inode * dirp;
	register struct inode_operations * iop;

	mode &= ~current->fs.umask;
	error = dir_namei(filename,&namelen,&basename, NULL, &dir);
	dirp = dir;
	if (error)
		return error;
	if (!namelen) {
		iput(dirp);
		return -ENOENT;
	}
	if (IS_RDONLY(dirp)) {
		iput(dirp);
		return -EROFS;
	}
	if ((error = permission(dirp,MAY_WRITE | MAY_EXEC)) != 0) {
		iput(dirp);
		return error;
	}
	iop = dirp->i_op;
	if (!iop || !iop->mknod) {
		iput(dirp);
		return -EPERM;
	}
	dirp->i_count++;
#ifdef NOT_YET	
	down(&dirp->i_sem);
#endif	
	error = iop->mknod(dirp,basename,namelen,mode,dev);
#ifdef NOT_YET
	up(&dirp->i_sem);
#endif	
	iput(dirp);
	return error;
}
#endif /* CONFIG_FS_RO */

int sys_mknod(filename,mode,dev)
char * filename;
int mode;
dev_t dev;
{
#ifdef CONFIG_FS_RO
	return -EROFS;
#else
	int error;

	if (S_ISDIR(mode) || (!S_ISFIFO(mode) && !suser()))
		return -EPERM;
	switch (mode & S_IFMT) {
	case 0:
		mode |= S_IFREG;
		break;
	case S_IFREG: case S_IFCHR: case S_IFBLK: case S_IFIFO: case S_IFSOCK:
		break;
	default:
		return -EINVAL;
	}
	error = do_mknod(filename,mode,dev);
	return error;
#endif
}

int sys_mkdir(pathname,mode)
char *pathname;
int mode;
{
#ifdef CONFIG_FS_RO
	return -EROFS;
#else
	char * basename;
	size_t namelen;
	int error;
	struct inode * dir;
	register struct inode * dirp;
	register struct inode_operations * iop;

	printd_fsmkdir("mkdir: calling dir_namei\n");
	error = dir_namei(pathname,&namelen,&basename,NULL,&dir);
	dirp = dir;
	if (error)
		return error;
	printd_fsmkdir("mkdir: finished dir_namei\n");
	if (!namelen) {
		iput(dirp);
		return -ENOENT;
	}
	if (IS_RDONLY(dirp)) {
		iput(dirp);
		return -EROFS;
	}
	if ((error = permission(dirp,MAY_WRITE | MAY_EXEC)) != 0) {
		iput(dirp);
		return error;
	}
	iop = dirp->i_op;
	if (!iop || !iop->mkdir) {
		iput(dirp);
		return -EPERM;
	}
	dirp->i_count++;
#ifdef NOT_YET	
	down(&dirp->i_sem);
#endif	
	printd_fsmkdir("mkdir: calling dir->i_op->mkdir...\n");
	error = iop->mkdir(dir, basename, namelen, mode & 0777 & ~current->fs.umask);
#ifdef NOT_YET	
	up(&dirp->i_sem);
#endif	
	iput(dirp);
	printd_fsmkdir("mkdir: (probably) successful\n");
	return error;
#endif /* CONFIG_FS_RO */
}

#ifndef CONFIG_FS_RO
static int do_rmdir(name)
char * name;
{
	char * basename;
	size_t namelen;
	int error;
	struct inode * dir;
	register struct inode * dirp;
	register struct inode_operations * iop;

	error = dir_namei(name,&namelen,&basename,NULL,&dir);
	dirp = dir;
	if (error)
		return error;
	if (!namelen) {
		iput(dirp);
		return -ENOENT;
	}
	if (IS_RDONLY(dirp)) {
		iput(dirp);
		return -EROFS;
	}
	if ((error = permission(dirp,MAY_WRITE | MAY_EXEC)) != 0) {
		iput(dirp);
		return error;
	}
	iop = dirp->i_op;
	if (!iop || !iop->rmdir) {
		iput(dirp);
		return -EPERM;
	}
	return iop->rmdir(dirp,basename,namelen);
}
#endif /* CONFIG_FS_RO */

int sys_rmdir(pathname)
char * pathname;
{
#ifdef CONFIG_FS_RO
	return -EROFS;
#else
	int error;
	error = do_rmdir(pathname);
	return error;
#endif
}

#ifndef CONFIG_FS_RO
static int do_unlink(name)
char * name;
{
	char * basename;
	size_t namelen;
	int error;
	struct inode * dir;
	register struct inode * dirp;
	register struct inode_operations * iop;

	error = dir_namei(name,&namelen,&basename,NULL,&dir);
	dirp = dir;
	if (error)
		return error;
	if (!namelen) {
		iput(dirp);
		return -EPERM;
	}
	if (IS_RDONLY(dirp)) {
		iput(dirp);
		return -EROFS;
	}
	if ((error = permission(dirp,MAY_WRITE | MAY_EXEC)) != 0) {
		iput(dirp);
		return error;
	}
	iop = dirp->i_op;
	if (!iop || !iop->unlink) {
		iput(dirp);
		return -EPERM;
	}
	return iop->unlink(dir,basename,namelen);
}
#endif

int sys_unlink(pathname)
char * pathname;
{
#ifdef CONFIG_FS_RO
	return -EROFS;
#else
	int error;
	error=do_unlink(pathname);
	return error;
#endif
}

#ifndef CONFIG_FS_RO
static int do_symlink(oldname,newname)
char * oldname;
char * newname;
{
	struct inode * dir;
	char * basename;
	size_t namelen;
	int error;
	register struct inode * dirp;
	register struct inode_operations * iop;

	error = dir_namei(newname,&namelen,&basename,NULL,&dir);
	dirp = dir;
	if (error)
		return error;
	if (!namelen) {
		iput(dirp);
		return -ENOENT;
	}
	if (IS_RDONLY(dirp)) {
		iput(dirp);
		return -EROFS;
	}
	if ((error = permission(dirp,MAY_WRITE | MAY_EXEC)) != 0) {
		iput(dirp);
		return error;
	}
	iop = dirp->i_op;
	if (!iop || !iop->symlink) {
		iput(dirp);
		return -EPERM;
	}
	dirp->i_count++;
#ifdef NOT_YET	
	down(&dirp->i_sem);
#endif	
	error = iop->symlink(dirp,basename,namelen,oldname);
#ifdef NOT_YET	
	up(&dirp->i_sem);
#endif	
	iput(dirp);
	return error;
}
#endif /* CONFIG_FS_RO */

int sys_symlink(oldname,newname)
char * oldname;
char * newname;
{
#ifdef CONFIG_FS_RO
	return -EROFS;
#else
	int error;
	error = do_symlink(oldname,newname);
	return error;
#endif
}

#ifndef CONFIG_FS_RO
static int do_link(oldinode,newname)
register struct inode * oldinode;
char * newname;
{
	struct inode * dir;
	char * basename;
	size_t namelen;
	int error;
	register struct inode * dirp;

	error = dir_namei(newname,&namelen,&basename,NULL,&dir);
	dirp = dir;
	if (error) {
		iput(oldinode);
		return error;
	}
	if (!namelen) {
		iput(oldinode);
		iput(dirp);
		return -EPERM;
	}
	if (IS_RDONLY(dirp)) {
		iput(oldinode);
		iput(dirp);
		return -EROFS;
	}
	if (dirp->i_dev != oldinode->i_dev) {
		iput(dirp);
		iput(oldinode);
		return -EXDEV;
	}
	if ((error = permission(dirp,MAY_WRITE | MAY_EXEC)) != 0) {
		iput(dirp);
		iput(oldinode);
		return error;
	}
	if (!dirp->i_op || !dirp->i_op->link) {
		iput(dirp);
		iput(oldinode);
		return -EPERM;
	}
	dirp->i_count++;
#ifdef NOT_YET	
	down(&dirp->i_sem);
#endif	
	error = dirp->i_op->link(oldinode, dirp, basename, namelen);
#ifdef NOT_YET	
	up(&dirp->i_sem);
#endif	
	iput(dirp);
	return error;
}
#endif /* CONFIG_FS_RO */

int sys_link(oldname,newname)
char * oldname;
char * newname;
{
#ifdef CONFIG_FS_RO
	return -EROFS;
#else
	int error;
	struct inode * oldinode;

	error = namei(oldname, &oldinode,0,0);
	if (error)
		return error;
	error = do_link(oldinode,newname);
	return error;
#endif
}

/* This probably isn't a proper implementation of sys_rename, but we don't have
 * space for one, and I don't want to write one when I can make a simple 6-line
 * version like this one :) - Chad */

int sys_rename(oldname, newname)
register char * oldname;
char * newname;
{
#ifdef CONFIG_FS_RO
	return -EROFS;
#else
	int err;

	if (!(err = sys_link(oldname, newname)))
		return sys_unlink(oldname);
	else
		return err;
#endif
}

