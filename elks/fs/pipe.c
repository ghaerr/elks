/*
 *  elks/fs/pipe.c
 *
 * Copyright (C) 1991, 1992 Linux Torvalds
 * 		 1998 Alistair Riddoch
 */

#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/major.h>
#include <linuxmt/stat.h>
#include <linuxmt/errno.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/string.h>
#include <linuxmt/time.h>
#include <linuxmt/locks.h>
#include <linuxmt/mm.h>
#include <linuxmt/fs.h>
#include <linuxmt/kdev_t.h>
#include <linuxmt/wait.h>
#include <linuxmt/debug.h>

#include <arch/system.h>
#include <arch/segment.h>
#include <arch/bitops.h>

#define MAX_PIPES 8

int get_unused_fd(void)
{
    register char *pfd = 0;

    do {
	if (!current->files.fd[(unsigned int) pfd]) {
	    (void) clear_bit((unsigned int) pfd,
			     &current->files.close_on_exec);
	    return (int) pfd;
	}
	++pfd;
    } while (((int)pfd) < NR_OPEN);

    return -EMFILE;
}

loff_t pipe_lseek(struct inode *inode, struct file *file, loff_t offset,
		  int orig)
{
    debug("PIPE: lseek called.\n");
    return -ESPIPE;
}

#ifdef CONFIG_PIPE

/*
 *	FIXME - we should pull pipes off the root (or pipe ?) fs as per
 *	V7, and they should be buffers
 */

char pipe_base[MAX_PIPES][PIPE_BUF];

int pipe_in_use[MAX_PIPES] = /*@i1@*/ { 0, };

char *get_pipe_mem(void)
{
    register char *pi = 0;

    do {
	if (!pipe_in_use[(int)pi]) {
	    pipe_in_use[(int)pi] = 1;
	    return pipe_base[(int)pi];
	}
	++pi;
    } while (((int)pi) < MAX_PIPES);

    debug("PIPE: No more buffers.\n");		/* FIXME */
    return NULL;
}


static size_t pipe_read(register struct inode *inode, struct file *filp,
		     char *buf, int count)
{
    size_t chars = 0, size = 0, read = 0;
    register char *pipebuf;

    debug("PIPE: read called.\n");
    if (filp->f_flags & O_NONBLOCK) {
	if ((inode->u.pipe_i.lock))
	    return -EAGAIN;
	if (((inode->u.pipe_i.len) == 0))
	    return ((inode->u.pipe_i.writers)) ? -EAGAIN : 0;
    } else
	while (((inode->u.pipe_i.len) == 0) || (inode->u.pipe_i.lock)) {
	    if (((inode->u.pipe_i.len) == 0)) {
		if (!(inode->u.pipe_i.writers))
		    return 0;
	    }
	    if (current->signal)
		return -ERESTARTSYS;
	    interruptible_sleep_on(&(inode->u.pipe_i.wait));
	}
    (inode->u.pipe_i.lock)++;
    while (count > 0 && (size = (size_t) (inode->u.pipe_i.len))) {
	chars = (PIPE_BUF - (inode->u.pipe_i.start));
	if (chars > (size_t) count)
	    chars = (size_t) count;
	if (chars > size)
	    chars = size;
	read += chars;
	pipebuf = (inode->u.pipe_i.base) + (inode->u.pipe_i.start);
	(inode->u.pipe_i.start) += chars;
	(inode->u.pipe_i.start) &= (PIPE_BUF - 1);
	(inode->u.pipe_i.len) -= chars;
	count -= chars;
	memcpy_tofs(buf, pipebuf, chars);
	buf += chars;
    }
    (inode->u.pipe_i.lock)--;
    wake_up_interruptible(&(inode->u.pipe_i.wait));
    if (read) {
	inode->i_atime = CURRENT_TIME;
	return (int) read;
    }
    if ((inode->u.pipe_i.writers))
	return -EAGAIN;
    return 0;
}

static size_t pipe_write(register struct inode *inode, struct file *filp,
		      char *buf, int count)
{
    register char *pipebuf;
    size_t chars = 0, free = 0, written = 0;

    debug("PIPE: write called.\n");
    if (!(inode->u.pipe_i.readers)) {
	send_sig(SIGPIPE, current, 0);
	return -EPIPE;
    }

    free = (count <= PIPE_BUF) ? (size_t) count : 1;
    while (count > 0) {
	while (((PIPE_BUF - (inode->u.pipe_i.len)) < free)
	       || (inode->u.pipe_i.lock)) {
	    if (!(inode->u.pipe_i.readers)) {
		send_sig(SIGPIPE, current, 0);
		return written ? (int) written : -EPIPE;
	    }
	    if (current->signal)
		return written ? (int) written : -ERESTARTSYS;
	    if (filp->f_flags & O_NONBLOCK)
		return written ? (int) written : -EAGAIN;
	    interruptible_sleep_on(&(inode->u.pipe_i.wait));
	}
	(inode->u.pipe_i.lock)++;
	while (count > 0 && (free = (PIPE_BUF - (inode->u.pipe_i.len)))) {

	    chars = -(((inode->u.pipe_i.start) + (inode->u.pipe_i.len))
		      & (PIPE_BUF - 1)) + PIPE_BUF;

	    if (chars > (size_t) count)
		chars = (size_t) count;

	    if (chars > free)
		chars = free;

	    pipebuf = (((inode->u.pipe_i.start) + (inode->u.pipe_i.len))
		       & (PIPE_BUF - 1)) + (inode->u.pipe_i.base);

	    written += chars;
	    (inode->u.pipe_i.len) += chars;
	    count -= chars;
	    memcpy_fromfs(pipebuf, buf, chars);
	    buf += chars;
	}
	(inode->u.pipe_i.lock)--;
	wake_up_interruptible(&(inode->u.pipe_i.wait));
	free = 1;
    }
    inode->i_ctime = inode->i_mtime = CURRENT_TIME;

    return written;
}

static void pipe_read_release(register struct inode *inode, struct file *filp)
{
    debug("PIPE: read_release called.\n");
    (inode->u.pipe_i.readers)--;
    wake_up_interruptible(&(inode->u.pipe_i.wait));
}

static void pipe_write_release(register struct inode *inode, struct file *filp)
{
    debug("PIPE: write_release called.\n");
    (inode->u.pipe_i.writers)--;
    wake_up_interruptible(&(inode->u.pipe_i.wait));
}

static void pipe_rdwr_release(register struct inode *inode,
			      register struct file *filp)
{
    debug("PIPE: rdwr_release called.\n");

    if (filp->f_mode & FMODE_READ)
	(inode->u.pipe_i.readers)--;

    if (filp->f_mode & FMODE_WRITE)
	(inode->u.pipe_i.writers)--;

    wake_up_interruptible(&(inode->u.pipe_i.wait));
}

static int pipe_read_open(struct inode *inode, struct file *filp)
{
    debug("PIPE: read_open called.\n");
    (inode->u.pipe_i.readers)++;

    return 0;
}

static int pipe_write_open(struct inode *inode, struct file *filp)
{
    debug("PIPE: write_open called.\n");
    (inode->u.pipe_i.writers)++;

    return 0;
}

static int pipe_rdwr_open(register struct inode *inode,
			  register struct file *filp)
{
    debug("PIPE: rdwr called.\n");

    if (filp->f_mode & FMODE_READ)
	(inode->u.pipe_i.readers)++;

    if (filp->f_mode & FMODE_WRITE)
	(inode->u.pipe_i.writers)++;

    return 0;
}

static size_t bad_pipe_rw(struct inode *inode, struct file *filp,
		       char *buf, int count)
{
    debug("PIPE: bad rw called.\n");

    return -EBADF;
}

/*@-type@*/

struct file_operations read_pipe_fops = {
    pipe_lseek, pipe_read, bad_pipe_rw, NULL,	/* no readdir */
    NULL,			/* select */
    NULL,			/* ioctl */
    pipe_read_open, pipe_read_release,
};

struct file_operations write_pipe_fops = {
    pipe_lseek,
    bad_pipe_rw,
    pipe_write,
    NULL,			/* no readdir */
    NULL,			/* select */
    NULL,			/* ioctl */
    pipe_write_open,
    pipe_write_release,
};

struct file_operations rdwr_pipe_fops = {
    pipe_lseek,
    pipe_read,
    pipe_write,
    NULL,			/* no readdir */
    NULL,			/* select */
    NULL,			/* ioctl */
    pipe_rdwr_open,
    pipe_rdwr_release,
};

struct inode_operations pipe_inode_operations = {
    &rdwr_pipe_fops, NULL,	/* create */
    NULL,			/* lookup */
    NULL,			/* link */
    NULL,			/* unlink */
    NULL,			/* symlink */
    NULL,			/* mkdir */
    NULL,			/* rmdir */
    NULL,			/* mknod */
    NULL,			/* readlink */
    NULL,			/* follow_link */
#ifdef BLOAT_FS
    NULL,			/* bmap */
#endif
    NULL,			/* truncate */
#ifdef BLOAT_FS
    NULL			/* permission */
#endif
};

/*@+type@*/

int do_pipe(int *fd)
{
    struct inode *inode;
    register struct file *f1;
    register struct file *f2;
    int error = ENFILE;
    int i, j;

    f1 = get_empty_filp();
    if (!f1)
	goto no_files;

    f2 = get_empty_filp();
    if (!f2)
	goto close_f1;

    inode = get_pipe_inode();
    if (!inode)
	goto close_f12;

    error = get_unused_fd();
    if (error < 0)
	goto close_f12_inode;

    i = error;
    current->files.fd[i] = f1;

    error = get_unused_fd();
    if (error < 0)
	goto close_f12_inode_i;

    j = error;
    f1->f_inode = f2->f_inode = inode;

    /* read file */
    f1->f_pos = f2->f_pos = 0;
    f1->f_flags = O_RDONLY;
    f1->f_op = &read_pipe_fops;
    f1->f_mode = 1;

    /* write file */
    f2->f_flags = O_WRONLY;
    f2->f_op = &write_pipe_fops;
    f2->f_mode = 2;

    current->files.fd[j] = f2;
    fd[0] = i;
    fd[1] = j;

    return 0;

  close_f12_inode_i:
#if 0
    put_unused_fd(i);		/* Not sure this is needed */
#endif

    current->files.fd[i] = NULL;

  close_f12_inode:inode->i_count--;
    iput(inode);

  close_f12:f2->f_count--;

  close_f1:f1->f_count--;

  no_files:return error;
}

int sys_pipe(unsigned int *filedes)
{
    int fd[2];
    int error;

    debug("PIPE: called.\n");

    if ((error = do_pipe(fd)))
	return error;

    debug2("PIPE: Returned %d %d.\n", fd[0], fd[1]);

    return verified_memcpy_tofs(filedes, fd, 2 * sizeof(int));
}

#else

int sys_pipe(unsigned int *filedes)
{
    return -ENOSYS;
}

#endif
