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
 * These functions are not defined or implemented yet.
 * I have not yet had a chance to verify whether they are required.
 */

#define down(_a)
#define up(_a)

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
	int error = -EACCES;

	if ((mask & MAY_WRITE) && (IS_RDONLY(inode))) {
		error = -EROFS;
	}
#ifdef BLOAT_FS
	else if (inode->i_op && inode->i_op->permission)
		error = inode->i_op->permission(inode, mask);
	else if ((mask & S_IWOTH) && IS_IMMUTABLE(inode))
		error = -EACCES; /* Nobody gets write access to an immutable file */
#else
#ifdef HAVE_IMMUTABLE
	else if ((mask & S_IWOTH) && IS_IMMUTABLE(inode)) 
		error = -EACCES; /* Nobody gets write access to an immutable file */
#endif
#endif
	else {
		if (current->euid == inode->i_uid)
			mode >>= 6;
		else if (in_group_p(inode->i_gid))
			mode >>= 3;
		if (((mode & mask & 0007) == mask) || suser())
			error = 0;
	}
	return error;
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
	int perm, retval = 0;

	*result = NULL;
	if (dir) {
	/* check permissions before traversing mount-points */
		perm = permission(dir,MAY_EXEC);
		if (len==2 && get_fs_byte(name) == '.' && get_fs_byte(name+1) == '.') {
			if (dir == current->fs.root) {
				*result = dir;
				goto lkp_end;
			} else if ((dir->i_sb) && (dir == dir->i_sb->s_mounted)) {
				iput(dir);
				dir = dir->i_sb->s_covered;
				if (!dir) {
					retval = -ENOENT;
					goto lkp_end;
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
			retval = -ENOTDIR;
		} else if (perm != 0) {
			iput(dir);
			retval = perm;
		} else if (!len) {
			*result = dir;
		} else retval = iop->lookup(dir,name,len,result);
	} else retval = -ENOENT;
lkp_end:
	return retval;
}

int follow_link(dir,inode,flag,mode,res_inode)
struct inode * dir;
register struct inode * inode;
int flag;
int mode;
struct inode ** res_inode;
{
	int error;
	register struct inode_operations * iop = inode->i_op;
	if (!dir || !inode) {
		iput(dir);
		iput(inode);
		*res_inode = NULL;
		error = -ENOENT;
	} else if (!iop || !iop->follow_link) {
		iput(dir);
		*res_inode = inode;
		error = 0;
	} else error = iop->follow_link(dir,inode,flag,mode,res_inode);
	return error;
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
	int error = 0;
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
			goto dnamei_end;
		}
		error = follow_link(base,inode,0,0,&base);
		if (error)
			goto dnamei_end;
	}
	if (!base->i_op || !base->i_op->lookup) {
		iput(base);
		error = -ENOTDIR;
	} else {
		*name = thisname;
		*namelen = len;
		*res_inode = base;
	}
#if 0	
	printk("namei: left dir_namei succesfully\n");
#endif
dnamei_end:
	return error;
}

int _namei(pathname,base,follow_links,res_inode)
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
	if (!error) {
		base->i_count++;	/* lookup uses up base */
		printd_namei("_namei: calling lookup\n");
		error = lookup(base,basename,namelen,&inode);
		printd_namei1("_namei: lookup returned %d\n", error);
		if (!error) {
			if (follow_links) {
				error = follow_link(base,inode,0,0,&inode);
				if (error)
					goto namei_end;
			} else iput(base);
			*res_inode = inode;
		} else iput(base);
	}
namei_end:
	return error;
}

#ifdef BLOAT_LNAMEI
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
#endif /* BLOAT_LNAMEI */

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
	if (!error) {
		inode=*res_inode;
		if (dir==NOT_DIR && S_ISDIR(inode->i_mode)) {
			error = -EISDIR;
		} else if (dir==IS_DIR && !S_ISDIR(inode->i_mode)) {
			error = -ENOTDIR;
		} else if (perm) {
			error=permission(inode, perm);
		}
		if (error != 0) {
			iput(inode);
		}
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
		goto onamei_end;
	if (!namelen) {			/* special case: '/usr/' etc */
		if (flag & 2) {
			iput(dir);
			error = -EISDIR;
		} else if ((error = permission(dir,ACC_MODE(flag))) != 0) {
		/* thanks to Paul Pluzhnikov for noticing this was missing.. */
			iput(dir);
		} else *res_inode=dir;
		goto onamei_end;
	}
	dir->i_count++;		/* lookup eats the dir */
	if (flag & O_CREAT) {
		down(&dir->i_sem);
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
		else {
			dir->i_count++;		/* create eats the dir */
			error = iop->create(dir,basename,namelen,mode,res_inode);
			up(&dir->i_sem);
			iput(dir);
			goto onamei_end;
		}
		up(&dir->i_sem);
	} else
		error = lookup(dir,basename,namelen,&inode);
	if (error) {
		iput(dir);
		goto onamei_end;
	}
	error = follow_link(dir,inode,flag,mode,&inode);
	if (error)
		goto onamei_end;
	if (S_ISDIR(inode->i_mode) && (flag & 2)) {
		error = -EISDIR;
	} else if ((error = permission(inode,ACC_MODE(flag))) == 0) {
		if (S_ISBLK(inode->i_mode) || S_ISCHR(inode->i_mode)) {
			if (IS_NODEV(inode)) {
				error = -EACCES;
			} else flag &= ~O_TRUNC;
		}
	}
	if (!error) {
		if (flag & O_TRUNC) {
			struct iattr newattrs;

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
			if ((iop = inode->i_op) && iop->truncate) {
				iop->truncate(inode);
			}
			up(&inode->i_sem);
			inode->i_dirt = 1;
			put_write_access(inode);
		}
		*res_inode = inode;
	} else iput(inode);
onamei_end:
	return error;
}

int sys_mknod(filename,mode,dev)
char * filename;
int mode;
dev_t dev;
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

	if (S_ISDIR(mode) || (!S_ISFIFO(mode) && !suser())) {
		error = -EPERM;
	} else {
		switch (mode & S_IFMT) {
		case 0:
			mode |= S_IFREG;
			break;
		case S_IFREG: case S_IFCHR: case S_IFBLK: case S_IFIFO: case S_IFSOCK:
			break;
		default:
			error = -EINVAL;
			goto mknod_end;
		}
		
		mode &= ~current->fs.umask;
		error = dir_namei(filename,&namelen,&basename, NULL, &dir);
		dirp = dir;
		if (error) goto mknod_end;
		if (!namelen) {
			error = -ENOENT;
		} else if ((error = permission(dirp,MAY_WRITE | MAY_EXEC)) == 0) {
			iop = dirp->i_op;
			if (!iop || !iop->mknod) {
				error = -EPERM;
			} else {
				dirp->i_count++;
				down(&dirp->i_sem);
				error = iop->mknod(dirp,basename,namelen,mode,dev);
				up(&dirp->i_sem);
			}
		}
		iput(dirp);
	}
mknod_end:
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
	printd_fsmkdir("mkdir: finished dir_namei\n");
	dirp = dir;
	if (!error) {
		if (!namelen) {
			error = -ENOENT;
		} else if ((error = permission(dirp,MAY_WRITE | MAY_EXEC)) == 0) {
			iop = dirp->i_op;
			if (!iop || !iop->mkdir) {
				error = -EPERM;
			} else {
				dirp->i_count++;
				down(&dirp->i_sem);
				printd_fsmkdir("mkdir: calling dir->i_op->mkdir...\n");
				error = iop->mkdir(dir, basename, namelen, mode & 0777 & ~current->fs.umask);
				up(&dirp->i_sem);
			}
		}
		iput(dirp);
		printd_fsmkdir("mkdir: complete\n");
	}
mkdir_end:
	return error;
#endif /* CONFIG_FS_RO */
}

int sys_rmdir(pathname)
char * pathname;
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

	error = dir_namei(pathname,&namelen,&basename,NULL,&dir);
	dirp = dir;
	if (!error) {
		if (!namelen) {
			error = -ENOENT;
		} else if ((error = permission(dirp,MAY_WRITE | MAY_EXEC)) == 0) {
			iop = dirp->i_op;
			if (!iop || !iop->rmdir) {
				error = -EPERM;
			}
		}
		if (error) {
			iput(dirp);
		} else error = iop->rmdir(dirp,basename,namelen);
	}
	return error;
#endif
}

int sys_unlink(pathname)
char * pathname;
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

	error = dir_namei(pathname,&namelen,&basename,NULL,&dir);
	dirp = dir;
	if (!error) {
		if (!namelen) {
			error = -EPERM;
		} else if ((error = permission(dirp,MAY_WRITE | MAY_EXEC)) == 0) {
			iop = dirp->i_op;
			if (!iop || !iop->unlink) {
				error = -EPERM;
			}
		}
		if (error) {
			iput(dirp);
		} else error = iop->unlink(dir,basename,namelen);
	}
	return error;
#endif
}

int sys_symlink(oldname,newname)
char * oldname;
char * newname;
{
#ifdef CONFIG_FS_RO
	return -EROFS;
#else
	struct inode * dir;
	char * basename;
	size_t namelen;
	int error;
	register struct inode * dirp;
	register struct inode_operations * iop;

	error = dir_namei(newname,&namelen,&basename,NULL,&dir);
	dirp = dir;
	if (!error) {
		if (!namelen) {
			error = -ENOENT;
		} else if ((error = permission(dirp,MAY_WRITE | MAY_EXEC)) == 0) {
			iop = dirp->i_op;
			if (!iop || !iop->symlink) {
				error = -EPERM;
			} else {
				dirp->i_count++;
				down(&dirp->i_sem);
				error = iop->symlink(dirp,basename,namelen,oldname);
				up(&dirp->i_sem);
			}
		}
		iput(dirp);
	}
	return error;
#endif
}

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
	if (!error) {
		struct inode * dir;
		char * basename;
		size_t namelen;
		register struct inode * dirp;

		error = dir_namei(newname,&namelen,&basename,NULL,&dir);
		dirp = dir;
		if (error) {
			goto link_fail;
		}
		if (!namelen) {
			error = -EPERM;
		} else if (dirp->i_dev != oldinode->i_dev) {
			error = -EXDEV;
		} else if ((error = permission(dirp,MAY_WRITE | MAY_EXEC)) == 0) {
			if (!dirp->i_op || !dirp->i_op->link) {
				error = -EPERM;
			}
		}
		if (!error) {
			dirp->i_count++;
			down(&dirp->i_sem);
			error = dirp->i_op->link(oldinode, dirp, basename, namelen);
			up(&dirp->i_sem);
		} else {
			iput(oldinode);
		}
link_fail:
		iput(dirp);
	} else {
		iput(oldinode);
	}
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

	if (!(err = sys_link(oldname, newname))) {
		err = sys_unlink(oldname);
	}
	return err;
#endif
}

