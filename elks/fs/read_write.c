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

int sys_lseek(unsigned int fd, loff_t * p_offset, unsigned int origin)
{
    register struct file *file;
    register struct file_operations *fop;
    loff_t offset;

    offset = (loff_t) get_user_long(p_offset);
    if (fd >= NR_OPEN || !(file = current->files.fd[fd]) || !(file->f_inode))
	return -EBADF;
    if (origin > 2) return -EINVAL;
    fop = file->f_op;
    if (fop && fop->lseek)
	return fop->lseek(file->f_inode, file, offset, origin);

    /* this is the default handler if no lseek handler is present */
    /* Note: We already determined above that origin is in range. */
    if (origin == 1)			/* SEEK_CUR */
	offset += file->f_pos;
    else if (origin)			/* SEEK_END */
	offset += file->f_inode->i_size;

    if (offset < 0) return -EINVAL;

    file->f_pos = offset;
    put_user_long((unsigned long int)offset, (void *)p_offset);

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

int fd_check(unsigned int fd, char *buf, size_t count, int rw, struct file **file)
{
    register struct file *tfil;

    if (fd >= NR_OPEN || !(tfil = current->files.fd[fd])
	|| !(tfil->f_inode) || !(tfil->f_mode & rw)) {
	return -EBADF;
    }

    if (!tfil->f_op) return -EINVAL;

    *file = tfil;
    if (count && verify_area(VERIFY_WRITE, buf, count)) return -EFAULT;

    return 0;
}

int sys_read(unsigned int fd, char *buf, size_t count)
{
    register struct file_operations *fop;
    struct file *file;
    int retval;

    if (((retval = fd_check(fd, buf, count, FMODE_READ, &file)) == 0) && count) {
	retval = -EINVAL;
	fop = file->f_op;
	if (fop->read) {
	    retval = (int) fop->read(file->f_inode, file, buf, count);
	    schedule();         // FIXME removing these slows down localhost networking
	}
    }
    return retval;
}

int sys_write(unsigned int fd, char *buf, size_t count)
{
    register struct file_operations *fop;
    struct file *file;
    register struct inode *inode;
    int written;

    if (((written = fd_check(fd, buf, count, FMODE_WRITE, &file)) == 0) && count) {
	written = -EINVAL;
	fop = file->f_op;
	if (fop->write) {
	    inode = file->f_inode;

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
	    written = (int) fop->write(inode, file, buf, count);
	    schedule();         // FIXME removing these slows down localhost networking
	}
    }
    return written;
}
