/*
 *  linux/fs/fcntl.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/types.h>
#include <arch/segment.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/fs.h>
#include <linuxmt/errno.h>
#include <linuxmt/stat.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/string.h>

static int dupfd(unsigned int fd, unsigned int arg)
{
    register struct file_struct *fils = &current->files;

    if (fd >= NR_OPEN || !fils->fd[fd])
	return -EBADF;

    if (arg >= NR_OPEN)
	return -EINVAL;

    while (arg < NR_OPEN)
	if (fils->fd[arg])
	    arg++;
	else
	    break;

    if (arg >= NR_OPEN)
	return -EMFILE;

    (void) clear_bit(arg, &fils->close_on_exec);
    (fils->fd[arg] = fils->fd[fd])->f_count++;

    return (int) arg;
}

int sys_dup2(unsigned int oldfd, unsigned int newfd)
{
    if (oldfd >= NR_OPEN || !current->files.fd[oldfd])
	return -EBADF;

    if (newfd == oldfd)
	return (int) newfd;

    if (newfd >= NR_OPEN)
	return -EBADF;		/* following POSIX.1 6.2.1 */

    sys_close(newfd);
    return dupfd(oldfd, newfd);
}

int sys_dup(unsigned int fildes)
{
    return dupfd(fildes, 0);
}

int sys_fcntl(unsigned int fd, unsigned int cmd, unsigned int arg)
{
    register struct file *filp;
    register struct file_struct *fils = &current->files;
    int result;

    if (fd >= NR_OPEN || !(filp = fils->fd[fd]))
	return -EBADF;

    switch (cmd) {
    case F_DUPFD:
	result = dupfd(fd, arg);
	break;
    case F_GETFD:
	result = test_bit(fd, &fils->close_on_exec);
	break;
    case F_SETFD:
	if (arg & 1)
	    (void) set_bit(fd, &fils->close_on_exec);
	else
	    (void) clear_bit(fd, &fils->close_on_exec);
	result = 0;
	break;
    case F_GETFL:
	result = (int) filp->f_flags;
	break;
    case F_SETFL:
	/*
	 * In the case of an append-only file, O_APPEND
	 * cannot be cleared
	 */
	if (IS_APPEND(filp->f_inode) && !(arg & O_APPEND))
	    result = -EPERM;
	else {
	    filp->f_flags &= ~(O_APPEND | O_NONBLOCK);
	    filp->f_flags |= arg & (O_APPEND | O_NONBLOCK);
	    result = 0;
	}
	break;
    default:
	result = -EINVAL;
	break;
    }

    return result;
}
