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
struct linux_dirent {
    u_ino_t d_ino;
    loff_t d_offset;
    size_t d_namlen;
    char d_name[255];
};

struct readdir_callback {
    struct linux_dirent *dirent;
    int count;
};

static int fillonedir(char *__buf, char *name, size_t namlen, off_t offset,
		      ino_t ino)
{
    register struct readdir_callback *buf = (struct readdir_callback *) __buf;
    register struct linux_dirent *dirent;

    if (buf->count)
	return -EINVAL;
    buf->count++;
    dirent = buf->dirent;
    put_user_long(ino, &dirent->d_ino);
    put_user_long(offset, &dirent->d_offset);
    put_user(namlen, (int *) &dirent->d_namlen);
    memcpy_tofs(dirent->d_name, name, namlen);
    put_user_char(0, dirent->d_name + namlen);
    return 0;
}

int sys_readdir(unsigned int fd, char *dirent, unsigned int count
		/* ignored and unused, noted in Linux man page */ )
{
    int error;
    struct file *file;
    register struct file_operations *fop;
    struct readdir_callback buf;

    error =
	fd_check(fd, dirent, sizeof(struct linux_dirent), FMODE_READ, &file);
    if (error < 0)
	return error;

    fop = file->f_op;
    if (!fop || !fop->readdir)
	return -ENOTDIR;

    buf.count = 0;
    buf.dirent = (struct linux_dirent *) dirent;
    error = fop->readdir(file->f_inode, file, &buf, fillonedir);

    if (error < 0)
	return error;

    return buf.count;
}
