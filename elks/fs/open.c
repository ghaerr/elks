/*
 *  linux/fs/open.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/vfs.h>
#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/stat.h>
#include <linuxmt/string.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/signal.h>
#include <linuxmt/time.h>
#include <linuxmt/fs.h>
#include <linuxmt/mm.h>
#include <linuxmt/utime.h>

#include <arch/segment.h>

#ifndef CONFIG_NOFS

#if 0
int sys_statfs(path,buf)
char * path;
register struct statfs * buf;
{
	register struct inode * inode;
	int error;
	struct statfs tmp;

	error = verify_area(VERIFY_WRITE, buf, sizeof(struct statfs));
	if (error)
		return error;
	error = namei(path,&inode,0,0);
	if (error)
		return error;
	if (!inode->i_sb->s_op->statfs_kern) {
		iput(inode);
		return -ENOSYS;
	}
	inode->i_sb->s_op->statfs_kern(inode->i_sb, &tmp, sizeof(struct statfs));
	iput(inode);
	memcpy(buf,&tmp,sizeof(tmp));
	return 0;
}

#ifndef CONFIG_FS_RO
int do_truncate(inode,length)
register struct inode *inode;
loff_t length;
{
	int error;
	struct iattr newattrs;
	register struct inode_operations * iop = inode->i_op;

/*	down(&inode->i_sem);*/
	newattrs.ia_size = length;
	newattrs.ia_valid = ATTR_SIZE | ATTR_CTIME;
	error = notify_change(inode, &newattrs);
	if (!error) {
		if (iop && iop->truncate)
			iop->truncate(inode);
	}
/*	up(&inode->i_sem);*/
	return error;
}

int sys_truncate(path,length)
char * path;
loff_t length;
{
	struct inode * inode;
	register struct inode * inodep;
	int error;

	error = namei(path,&inode, NOT_DIR, MAY_WRITE);
	inodep = inode;
	if (error)
		return error;
	if (IS_RDONLY(inodep)) {
		iput(inodep);
		return -EROFS;
	}
#ifdef BLOAT_FS
	error = get_write_access(inodep);
	if (error) {
		iput(inodep);
		return error;
	}
#endif
	error = do_truncate(inodep, length);
	put_write_access(inodep);
	iput(inodep);
	return error;
}

int sys_ftruncate(fd,length)
unsigned int fd;
loff_t length;
{
	register struct inode * inode;
	register struct file * file;

	if (fd >= NR_OPEN || !(file = current->files.fd[fd]))
		return -EBADF;
	if (!(inode = file->f_inode))
		return -ENOENT;
	if (S_ISDIR(inode->i_mode) || !(file->f_mode & FMODE_WRITE))
		return -EACCES;
	return do_truncate(inode, length);
}
#endif /* CONFIG_FS_RO */

/* If times==NULL, set access and modification to current time,
 * must be owner or have write permission.
 * Else, update from *times, must be owner or super user.
 */

int sys_utimes(filename,utimes)
char * filename;
struct timeval * utimes;
{
	int error;
	struct inode * inode;
	register struct inode * inodep;
	struct iattr newattrs;

	error = namei(filename,&inode,0,0);
	inodep = inode;
	if (error)
		return error;
	if (IS_RDONLY(inodep)) {
		iput(inodep);
		return -EROFS;
	}
	/* Don't worry, the checks are done in inode_change_ok() */
	newattrs.ia_valid = ATTR_CTIME | ATTR_MTIME | ATTR_ATIME;
	if (utimes) {
		struct timeval times[2];
		if (error = verified_memcpy_fromfs(&times, utimes, sizeof(times))) {
			iput(inodep);
			return error;
		}
		newattrs.ia_atime = times[0].tv_sec;
		newattrs.ia_mtime = times[1].tv_sec;
		newattrs.ia_valid |= ATTR_ATIME_SET | ATTR_MTIME_SET;
	} else {
		if ((error = permission(inodep,MAY_WRITE)) != 0) {
			iput(inodep);
			return error;
		}
	}
	error = notify_change(inodep, &newattrs);
	iput(inodep);
	return error;
}
#endif /* 0 */

int sys_utime(filename,times)
char * filename;
struct utimbuf * times;
{
	struct inode * inode;
	int error;
	time_t actime,modtime;

	error=namei(filename,&inode,0,0);
	if (error)
		return error;
#ifdef CONFIG_ACTIME
	if (times) {
		actime = get_fs_long((unsigned long *) &times->actime);
		modtime = get_fs_long((unsigned long *) &times->modtime);
	} else
		actime = modtime = CURRENT_TIME;
	inode->i_atime = actime;
#else
	if (times) {
		modtime = get_fs_long((unsigned long *) &times->modtime);
	} else
		modtime = CURRENT_TIME;
#endif
	inode->i_mtime = modtime;
	inode->i_dirt = 1;
	iput(inode);
	return 0;
}



/*
 * access() needs to use the real uid/gid, not the effective uid/gid.
 * We do this by temporarily setting fsuid/fsgid to the wanted values
 */

int sys_access(filename,mode)
char * filename;
int mode;
{
	struct inode * inode;
	register __ptask currentp = current;
	int old_euid, old_egid;
	int res;

	if (mode != (mode & S_IRWXO))	/* where's F_OK, X_OK, W_OK, R_OK? */
		return -EINVAL;
	old_euid = currentp->euid;
	old_egid = currentp->egid;
	currentp->euid = currentp->uid;
	currentp->egid = currentp->gid;
	res = namei(filename,&inode, 0, mode);
	currentp->euid = old_euid;
	currentp->egid = old_egid;
	return res;
}

int sys_chdir(filename)
char * filename;
{
	struct inode * inode;
	int error;

	error = namei(filename,&inode, IS_DIR, MAY_EXEC);
	if (error)
		return error;
	iput(current->fs.pwd);
	current->fs.pwd = inode;
	return (0);
}

int sys_chroot(filename)
char * filename;
{
	struct inode * inode;
	int error;

	error = namei(filename,&inode, IS_DIR, 0);
	if (error)
		return error;
	if (!suser()) {
		iput(inode);
		return -EPERM;
	}
	iput(current->fs.root);
	current->fs.root = inode;
	return (0);
}

int sys_chmod(filename,mode)
char * filename;
mode_t mode;
{
#ifdef USE_NOTIFY_CHANGE
	struct inode * inode;
	register struct inode * inodep;
	int error;
	struct iattr newattrs;
	register struct iattr * nap = &newattrs;

	error = namei(filename,&inode,0,0);
	inodep = inode;
	if (error)
		return error;
	if (IS_RDONLY(inodep)) {
		iput(inodep);
		return -EROFS;
	}
	if (mode == (mode_t) -1)
		mode = inodep->i_mode;
	nap->ia_mode = (mode & S_IALLUGO) | (inodep->i_mode & ~S_IALLUGO);
	nap->ia_valid = ATTR_MODE | ATTR_CTIME;
	inodep->i_dirt = 1;
	error = notify_change(inodep, nap);
	iput(inodep);
	return error;
#else
	struct inode * inode;
	register struct inode * inodep;
	int error;

	error = namei(filename,&inode,0,0);
	inodep = inode;
	if (error)
		return error;
	if (IS_RDONLY(inodep)) {
		iput(inodep);
		return -EROFS;
	}
	if (mode == (mode_t) -1)
		mode = inodep->i_mode;
	mode = (mode & S_IALLUGO) | (inodep->i_mode & ~S_IALLUGO);
	if ((current->euid != inodep->i_uid) && !suser()) {
		return -EPERM;
	}
	if (!suser() && !in_group_p(inodep->i_gid)) {
		mode &= ~S_ISGID;
	}
	inodep->i_mode = mode;
	inodep->i_dirt = 1;
	iput(inodep);
	return error;
#endif
}

static int do_chown(inode, user, group)
register struct inode * inode;
uid_t user;
gid_t group;
{
#ifdef USE_NOTIFY_CHANGE
	int error;
	struct iattr newattrs;
	register struct iattr * nap = &newattrs;

	if (IS_RDONLY(inode)) {
		return -EROFS;
	}
	if (user == (uid_t) -1)
		user = inode->i_uid;
	if (group == (gid_t) -1)
		group = inode->i_gid;
	nap->ia_mode = inode->i_mode;
	nap->ia_uid = user;
	nap->ia_gid = group;
	nap->ia_valid =  ATTR_UID | ATTR_GID | ATTR_CTIME;
	/*
	 * If the owner has been changed, remove the setuid bit
	 */
	if (user != inode->i_uid && (inode->i_mode & S_ISUID)) {
		nap->ia_mode &= ~S_ISUID;
		nap->ia_valid |= ATTR_MODE;
	}
	/*
	 * If the group has been changed, remove the setgid bit
	 */
	if (group != inode->i_gid && (inode->i_mode & S_ISGID)) {
		nap->ia_mode &= ~S_ISGID;
		nap->ia_valid |= ATTR_MODE;
	}
	inode->i_dirt = 1;
	error = notify_change(inode, nap);
	return(error);
#else
	if (IS_RDONLY(inode)) {
		return -EROFS;
	}
	if (!suser() && (current->euid != inode->i_uid)) {
		return -EROFS;
	}
	if (group != (gid_t) -1) {
		inode->i_gid = group; 
		inode->i_mode &= ~S_ISGID;
	}
	if (user != (uid_t) -1) {
		inode->i_uid = user;
		inode->i_mode &= ~S_ISUID;
	}
	inode->i_dirt = 1;
	return(0);
#endif
}

int sys_chown(filename,user,group)
char * filename;
uid_t user;
gid_t group;
{
	struct inode * inode;
	int error;

	error = lnamei(filename,&inode);
	if (error)
		return error;

	error = do_chown(inode, user, group);
	iput(inode);
	return error;
}

int sys_fchown(fd, user, group)
unsigned int fd;
uid_t user;
gid_t group;
{
	register struct file * filp;

	if (fd >= NR_OPEN || !(filp=current->files.fd[fd]) || !(filp->f_inode))
		return -EBADF;

	return do_chown(filp->f_inode, user, group);
}

#endif /* CONFIG_NOFS */

/*
 * Note that while the flag value (low two bits) for sys_open means:
 *	00 - read-only
 *	01 - write-only
 *	10 - read-write
 *	11 - special
 * it is changed into
 *	00 - no permissions needed
 *	01 - read-permission
 *	10 - write-permission
 *	11 - read-write
 * for the internal routines (ie open_namei()/follow_link() etc). 00 is
 * used by symlinks.
 */
 
static int do_open(filename,flags,mode)
char * filename;
int flags;
int mode;
{
	struct inode * inode;
	register struct file * f;
	int flag,error,fd;

	f = get_empty_filp();
	if (!f) {
		printk("\nNo filps\n");
		return -ENFILE;
	}
	f->f_flags = flag = flags;
	f->f_mode = (flag+1) & O_ACCMODE;
	if (f->f_mode)
		flag++;
	if (flag & (O_TRUNC | O_CREAT))
		flag |= 2;
	error = open_namei(filename,flag,mode,&inode,NULL);

	if (error) {
		goto cleanup_file;
	}
#ifdef BLOAT_FS
	if (f->f_mode & FMODE_WRITE) {
		error = get_write_access(inode);
		if (error)
			goto cleanup_inode;
	}
#endif

	f->f_inode = inode;
	f->f_pos = 0;
#ifdef BLOAT_FS
	f->f_reada = 0;
#endif
	f->f_op = NULL;
	{
	register struct inode_operations * iop = inode->i_op;
	if (iop)
		f->f_op = iop->default_file_ops;
	}
	{
	register struct file_operations * fop = f->f_op;
	if (fop && fop->open) {
		error = fop->open(inode,f);
		if (error) {
			goto cleanup_all;
		}
	}
	f->f_flags &= ~(O_CREAT | O_EXCL | O_NOCTTY | O_TRUNC);

	/*
	 * We have to do this last, because we mustn't export
	 * an incomplete fd to other processes which may share
	 * the same file table with us.
	 * FIXME - replace with call to get_unused_fd()
	 */
/*	for(fd = 0; fd < NR_OPEN; fd++) {
		if (!current->files.fd[fd]) {
			current->files.fd[fd] = f;
			clear_bit(fd,&current->files.close_on_exec);
			return fd;
		}
	} */
	if ((fd = get_unused_fd()) > -1) {
		current->files.fd[fd] = f;
		return fd;
	}
	error = -EMFILE;
	if (fop && fop->release)
		fop->release(inode,f);
	}
cleanup_all:
#ifdef BLOAT_FS
	if (f->f_mode & FMODE_WRITE)
		put_write_access(inode);
cleanup_inode:
#endif
	iput(inode);
cleanup_file:
	f->f_count--;
	return error;
}

int sys_open(filename,flags,mode)
char * filename;
int flags;
int mode;
{
	int error;

	/* FIXME: VERIFY THE PATH IS IN USER MEM FIRST */
	error = do_open(filename,flags,mode);
	return error;
}

static int close_fp(filp)
register struct file *filp;
{
	register struct inode *inode;

	if (filp->f_count == 0) {
		printk("VFS: Close: file count is 0\n");
		return 0;
	}
	if (filp->f_count > 1) {
		filp->f_count--;
		return 0;
	}
	inode = filp->f_inode;
	if (filp->f_op && filp->f_op->release)
		filp->f_op->release(inode,filp);
	filp->f_count--;
	filp->f_inode = NULL;
#ifdef BLOAT_FS
	if (filp->f_mode & FMODE_WRITE)
		put_write_access(inode);
#endif
	iput(inode);
	return 0;
}

/* This is used by exit and sometimes exec to close all files of a process */

void _close_allfiles()
{
	int i;
	register struct file * filp;

	for (i = 0; i < NR_OPEN; i++) {
		if ((filp = current->files.fd[i])) {
			current->files.fd[i] = NULL;
			close_fp(filp);
		}
	}
}

int sys_close(fd)
unsigned int fd;
{	
	register struct file * filp;
	register struct file_struct * cfiles = &current->files;

	if (fd >= NR_OPEN)
		return -EBADF;
	clear_bit(fd, &cfiles->close_on_exec);
	if (!(filp = cfiles->fd[fd]))
		return -EBADF;
	cfiles->fd[fd] = NULL;
	return (close_fp (filp));
}
