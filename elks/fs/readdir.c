/*
 *  linux/fs/readdir.c
 *
 *  Copyright (C) 1995  Linus Torvalds
 *
 * THIS NEEDS CLEANUP! - Chad
 */

#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/stat.h>
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/mm.h>
#include <linuxmt/debug.h>

#include <arch/segment.h>

/*
 * Traditional linux readdir() handling..
 *
 */
#define NAME_OFFSET(de) ((int) ((de)->d_name - (char *) (de)))
#define ROUND_UP(x) (((x)+sizeof(long)-1) & ~(sizeof(long)-1))

struct linux_dirent {
	unsigned long	d_ino;
	unsigned long	d_offset;
	unsigned short	d_namlen;
	char		d_name[255];
};

struct readdir_callback 
{
	struct linux_dirent * dirent;
	int count;
};

static int fillonedir(__buf,name,namlen,offset,ino)
char *__buf;
char * name;
int namlen;
off_t offset;
ino_t ino;
{
	register struct readdir_callback * buf = (struct readdir_callback *) __buf;
	register struct linux_dirent * dirent;

	if (buf->count)
		return -EINVAL;
	buf->count++;
	dirent = buf->dirent;
	put_user(ino, &dirent->d_ino);
	put_user(offset, &dirent->d_offset);
	put_user(namlen, &dirent->d_namlen);
	memcpy_tofs(dirent->d_name, name, namlen);
	put_user(0, dirent->d_name + namlen);
	return 0;
}

int sys_readdir(fd,dirent,count)
unsigned int fd;
char * dirent;
unsigned int count;
{
	int error;
	register struct file * file;
	register struct file_operations * fop;
	struct readdir_callback buf;

	if (fd >= NR_OPEN || !(file = current->files.fd[fd]))
		return -EBADF;
	fop=file->f_op;
	if (!fop || !fop->readdir)
		return -ENOTDIR;
	error = verify_area(VERIFY_WRITE, dirent, sizeof(struct linux_dirent));
	if (error)
		return error;
	buf.count = 0;
	buf.dirent = dirent;
	error = fop->readdir(file->f_inode, file, &buf, fillonedir);
	if (error < 0)
		return error;
	return buf.count;
}

