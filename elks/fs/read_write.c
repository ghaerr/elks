/*
 *  linux/fs/read_write.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/stat.h>
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/mm.h>
#include <linuxmt/fs.h>
#include <arch/segment.h>
#include <linuxmt/debug.h>

int sys_lseek(fd,p_offset,origin)
unsigned int fd;
off_t * p_offset;
unsigned int origin;
{
	off_t offset = get_fs_long(p_offset);
	register struct file * file;
	register struct file_operations * fop;
	off_t tmp;
	
	if (fd >= NR_OPEN || !(file=current->files.fd[fd]) || !(file->f_inode))
		return -EBADF;
	if (origin > 2)
		return -EINVAL;
	fop = file->f_op;
	if (fop && fop->lseek)
		return fop->lseek(file->f_inode,file,offset,origin);

/* this is the default handler if no lseek handler is present */
	switch (origin) {
		case 0:
			tmp = offset;
			break;
		case 1:
			tmp = file->f_pos + offset;
			break;
		case 2:
			if (!file->f_inode)
				return -EINVAL;
			tmp = file->f_inode->i_size + offset;
			break;
		default:
		/* bogus origin */
			return -EINVAL;
	}
/*	printk("%d lseek3\n",fd); */

/* Cannot be nagative as unsigend. Should off_t be unsigned? */
/*	if (tmp < 0)
		return -EINVAL; */
	if (tmp != file->f_pos) {
		file->f_pos = tmp;
#ifdef BLOAT_FS
		file->f_reada = 0;
		file->f_version = ++event;
#endif
	}
	return 0;
}

/* fd_check -- validate file descriptor
 *  Failure is indicated by a returning a non-zero value. Success is
 *  indicated by returning 0. The parameter "file" is used for passing
 *  back the pointer to the file struct associated with fd. The value of
 *  "file" is undefined when this function returns unsuccessfully.
 *
 *  Possible error values are:
 *    EBADF : fd is not a valid file descriptor or is not open for reading.
 *    EINVAL: fd is attached to an object which is unsuitable for reading.
 *    EFAULT: buf is outside your accessible address space.
 */

int fd_check(fd, buf, count, rw, file)
unsigned int fd;
char * buf;
unsigned int count;
int rw;
struct file ** file;
{
	register struct file * tfil;
	int error;

	if (fd>=NR_OPEN || !(tfil=current->files.fd[fd]) || !(tfil->f_inode)) {
		return -EBADF;
	}
	if (!(tfil->f_mode & rw)) {
		return -EBADF;
	}
	if (!tfil->f_op) {
		return -EINVAL;
	}

	*file = tfil;
	if (count && (error = verify_area(VERIFY_WRITE, buf, count))) {
		return error;
	}
	return 0;
}
	

int sys_read(fd,buf,count)
unsigned int fd;
char * buf;
unsigned int count;
{
	int retval;
	register struct file_operations * fop;
	struct file * file;

	if (((retval = fd_check(fd, buf, count, FMODE_READ, &file)) != 0)
	    || (0 == count)) {
		return retval;
	}
	fop = file->f_op;
	if (!fop->read) {
		return -EINVAL;
	}
	
	retval = fop->read(file->f_inode,file,buf,count);
	schedule();
	return retval;
}

int sys_write(fd,buf,count)
unsigned int fd;
char * buf;
unsigned int count;
{
	struct file * file;
	register struct inode * inode;
	int written;

	if (((written = fd_check(fd, buf, count, FMODE_WRITE, &file)) != 0)
	    || (0 == count)) {
		return written;
	}
	if (!file->f_op->write) {
		return -EINVAL;
	}
	inode=file->f_inode;

#ifndef CONFIG_NOFS
	/*
	 * If data has been written to the file, remove the setuid and
	 * the setgid bits. We do it anyway otherwise there is an
	 * extremely exploitable race - does your OS get it right |->
	 *
	 * Set ATTR_FORCE so it will always be changed.
	 */
	if (!suser() && (inode->i_mode & (S_ISUID | S_ISGID))) {
#ifdef USE_NOTIFY_CHANGE
		struct iattr newattrs;
		newattrs.ia_mode = inode->i_mode & ~(S_ISUID | S_ISGID);
		newattrs.ia_valid = ATTR_CTIME | ATTR_MODE | ATTR_FORCE;
		notify_change(inode, &newattrs);
#else
		inode->i_mode = inode->i_mode & ~(S_ISUID | S_ISGID);
#endif
	}
#endif

	written = file->f_op->write(inode,file,buf,count);
	schedule();
	return written;
}

