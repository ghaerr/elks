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

int dupfd(fd,arg)
unsigned int fd;
unsigned int arg;
{
	register struct file_struct * fils = &current->files;

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

int sys_dup2(oldfd,newfd)
unsigned int oldfd;
unsigned int newfd;
{
	if (oldfd >= NR_OPEN || !current->files.fd[oldfd])
		return -EBADF;
	if (newfd == oldfd)
		return newfd;
	if (newfd >= NR_OPEN)
		return -EBADF;	/* following POSIX.1 6.2.1 */

	sys_close(newfd);
	return dupfd(oldfd,newfd);
}

int sys_dup(fildes)
unsigned int fildes;
{
	return dupfd(fildes,0);
}

long sys_fcntl(fd,cmd,arg)
unsigned int fd;
unsigned int cmd;
unsigned long arg;
{	
	register struct file * filp;
	register struct file_struct * fils = &current->files;
	struct task_struct *p;
	int task_found = 0;

	if (fd >= NR_OPEN || !(filp = fils->fd[fd]))
		return -EBADF;
	switch (cmd) {
		case F_DUPFD:
			return dupfd(fd,arg);
		case F_GETFD:
			return test_bit(fd, &fils->close_on_exec);
		case F_SETFD:
			if (arg&1)
				set_bit(fd, &fils->close_on_exec);
			else
				clear_bit(fd, &fils->close_on_exec);
			return 0;
		case F_GETFL:
			return filp->f_flags;
		case F_SETFL:
			/*
			 * In the case of an append-only file, O_APPEND
			 * cannot be cleared
			 */
			if (IS_APPEND(filp->f_inode) && !(arg & O_APPEND))
				return -EPERM;
			filp->f_flags &= ~(O_APPEND | O_NONBLOCK);
			filp->f_flags |= arg & (O_APPEND | O_NONBLOCK);
			return 0;
		default:
			return -EINVAL;
	}
}

