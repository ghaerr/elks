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

int dupfd(int fd, int arg)
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

    clear_bit(arg, &fils->close_on_exec);
    (fils->fd[arg] = fils->fd[fd])->f_count++;

    return arg;
}

int sys_dup2(int oldfd, int newfd)
{
    if (oldfd >= NR_OPEN || !current->files.fd[oldfd])
	return -EBADF;

    if (newfd == oldfd)
	return newfd;

    if (newfd >= NR_OPEN)
	return -EBADF;		/* following POSIX.1 6.2.1 */

    sys_close(newfd);
    return dupfd(oldfd, newfd);
}

int sys_dup(int fildes)
{
    return dupfd(fildes, 0);
}

int sys_fcntl(int fd, unsigned int cmd, int arg)
{
    register struct file *filp;
    register struct file_struct *fils = &current->files;

    if (fd >= NR_OPEN || !(filp = fils->fd[fd]))
	return -EBADF;

    switch (cmd) {
    case F_DUPFD:
	arg = dupfd(fd, arg);
	break;
    case F_GETFD:
	arg = test_bit(fd, &fils->close_on_exec);
	break;
    case F_SETFD:
	if (arg & 1)
	    set_bit(fd, &fils->close_on_exec);
	else
	    clear_bit(fd, &fils->close_on_exec);
	arg = 0;
	break;
    case F_GETFL:
	arg = filp->f_flags;
	break;
    case F_SETFL:
	/*
	 * In the case of an append-only file, O_APPEND
	 * cannot be cleared
	 */
	if (IS_APPEND(filp->f_inode) && !(arg & O_APPEND))
	    return -EPERM;
	filp->f_flags &= ~(O_APPEND | O_NONBLOCK);
	filp->f_flags |= arg & (O_APPEND | O_NONBLOCK);
	arg = 0;
	break;
    default:
	arg = -EINVAL;
    }

    return arg;
}
