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
	off_t tmp = -1;
	
/*	printk("%d lseek1\n",origin); */
/*	printk("%ld lseek2\n",offset); */
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
	}
/*	printk("%d lseek3\n",fd); */

	if (tmp < 0)
		return -EINVAL;
	if (tmp != file->f_pos) {
		file->f_pos = tmp;
#ifdef BLOAT_FS
		file->f_reada = 0;
		file->f_version = ++event;
#endif
	}
	return 0;
}

int sys_read(fd,buf,count)
unsigned int fd;
char * buf;
unsigned int count;
{
	int error, retval;
	register struct file * file;
	struct inode * inode;
	register struct file_operations * fop;

	if (fd>=NR_OPEN || !(file=current->files.fd[fd]) || !(inode=file->f_inode))
		return -EBADF;
	if (!(file->f_mode & 1))
		return -EBADF;
	fop = file->f_op;
	if (!fop || !fop->read)
	{
		printk("readwrite: we're analphabetic\n");
		return -EINVAL;
	}

	if (!count)
		return 0;
	error = verify_area(VERIFY_WRITE,buf,count);
	if (error)
		return error;
	retval = fop->read(inode,file,buf,count);
	schedule();
	return retval;
}

int sys_write(fd,buf,count)
unsigned int fd;
char * buf;
unsigned int count;
{
	int error;
	register struct file * file;
	register struct inode * inode;
	int written;

/*	printk("{WRITE: %d %x %d, %x:%x.}", fd, buf, count, current->t_regs.cs, current->t_regs.ip); /* */
	if (fd>=NR_OPEN || !(file=current->files.fd[fd]) || !(inode=file->f_inode))
		return -EBADF;
	if (!(file->f_mode & 2))
		return -EBADF;
	if (!file->f_op || !file->f_op->write)
		return -EINVAL;
	if (!count)
		return 0;
	error = verify_area(VERIFY_READ,buf,count);
	if (error)
		return error;
	/*
	 * If data has been written to the file, remove the setuid and
	 * the setgid bits. We do it anyway otherwise there is an
	 * extremely exploitable race - does your OS get it right |->
	 *
	 * Set ATTR_FORCE so it will always be changed.
	 */
	if (!suser() && (inode->i_mode & (S_ISUID | S_ISGID))) {
		struct iattr newattrs;
		newattrs.ia_mode = inode->i_mode & ~(S_ISUID | S_ISGID);
		newattrs.ia_valid = ATTR_CTIME | ATTR_MODE | ATTR_FORCE;
		notify_change(inode, &newattrs);
	}

/*	down(&inode->i_sem);*/
	written = file->f_op->write(inode,file,buf,count);
/*	up(&inode->i_sem);*/
	schedule();
	return written;
}

