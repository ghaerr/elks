/*
 *  linux/fs/ioctl.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/types.h>
#include <arch/segment.h>
#include <linuxmt/sched.h>
#include <linuxmt/mm.h>
#include <linuxmt/errno.h>
#include <linuxmt/string.h>
#include <linuxmt/stat.h>
#include <linuxmt/termios.h>
#include <linuxmt/fcntl.h> /* for f_flags values */

static int file_ioctl(filp,cmd,arg)
register struct file *filp;
unsigned int cmd;
unsigned int arg;
{
	long val;
	int block;

	switch (cmd) {
		case FIONREAD:
			val = filp->f_inode->i_size - filp->f_pos;
			return verified_memcpy_tofs(arg, &val, sizeof(long));
	}
	if (filp->f_op && filp->f_op->ioctl)
		return filp->f_op->ioctl(filp->f_inode, filp, cmd, arg);
	return -EINVAL;
}


int sys_ioctl(fd,cmd,arg)
unsigned int fd;
unsigned int cmd;
unsigned int arg;
{	
/*	unsigned long arg = get_fs_long(parg); */
	register struct file * filp;
	int on;

	if (fd >= NR_OPEN || !(filp = current->files.fd[fd]))
		return -EBADF;
	switch (cmd) {
		case FIOCLEX:
			FD_SET(fd, &current->files.close_on_exec);
			return 0;

		case FIONCLEX:
			FD_CLR(fd, &current->files.close_on_exec);
			return 0;

		case FIONBIO:
			memcpy_fromfs(&on, (unsigned int *) arg, 2);
			/* on = get_fs_word((unsigned int *) arg); */
			if (on)
				filp->f_flags |= O_NONBLOCK;
			else
				filp->f_flags &= ~O_NONBLOCK;
			return 0;

		default:
			if (filp->f_inode && S_ISREG(filp->f_inode->i_mode))
				return file_ioctl(filp, cmd, arg);

			if (filp->f_op && filp->f_op->ioctl)
				return filp->f_op->ioctl(filp->f_inode, filp, cmd, arg);

			return -EINVAL;
	}
}
