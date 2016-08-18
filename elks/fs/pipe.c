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

int get_unused_fd(struct file *f)
{
    register char *pfd = 0;
    register struct file_struct *cfs = &current->files;

    do {
	if (!cfs->fd[(unsigned int) pfd]) {
	    cfs->fd[(unsigned int) pfd] = f;
	    clear_bit((unsigned int) pfd,
			     &cfs->close_on_exec);
	    return (int) pfd;
	}
    } while (((int)(++pfd)) < NR_OPEN);

    return -EMFILE;
}

int open_fd(int flags, register struct inode *inode)
{
    int fd;
    struct file *filp;

    if (!(fd = open_filp(flags, inode, &filp))
	&& ((fd = get_unused_fd(filp)) < 0))
	close_filp(inode, filp);
    return fd;
}

int pipe_lseek(struct inode *inode, struct file *file, loff_t offset,
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

static char pipe_base[MAX_PIPES][PAGE_SIZE];

static int pipe_in_use[(MAX_PIPES + 15)/16];

static char *get_pipe_mem(void)
{
    register char *i = 0;

    i = (char *)find_first_zero_bit(pipe_in_use, MAX_PIPES);
    if ((int)i < MAX_PIPES) {
	set_bit((int)i, pipe_in_use);
	return pipe_base[(int)i];
    }

    debug("PIPE: No more buffers.\n");		/* FIXME */
    return NULL;
}

static void free_pipe_mem(char *buf)
{
    register char *i;

    i = (char *)(((unsigned int)pipe_base - (unsigned int)buf)/PAGE_SIZE);
    if ((int)i < MAX_PIPES)
	clear_bit((int)i, pipe_in_use);
}

static size_t pipe_read(register struct inode *inode, struct file *filp,
		     char *buf, size_t count)
{
    register char *chars;

    debug("PIPE: read called.\n");
    while (!(inode->u.pipe_i.q.len) || (inode->u.pipe_i.lock)) {
	if (!(inode->u.pipe_i.lock) && !(inode->u.pipe_i.writers)) {
	    return 0;
	}
	if (filp->f_flags & O_NONBLOCK) {
	    return -EAGAIN;
	}
	if (current->signal)
	    return -ERESTARTSYS;
	interruptible_sleep_on(&(inode->u.pipe_i.q.wait));
    }
    (inode->u.pipe_i.lock)++;
    if (count > inode->u.pipe_i.q.len)
	count = inode->u.pipe_i.q.len;
    chars = (char *)(PIPE_BUF - inode->u.pipe_i.q.start);
    if ((size_t)chars > count)
	chars = (char *)count;
    memcpy_tofs(buf, (inode->u.pipe_i.q.base+inode->u.pipe_i.q.start), (size_t)chars);
    if ((size_t)chars < count)
	memcpy_tofs(buf + (size_t)chars, inode->u.pipe_i.q.base, count - (size_t)chars);
    inode->u.pipe_i.q.start = (inode->u.pipe_i.q.start + count) & (PIPE_BUF - 1);
    inode->u.pipe_i.q.len -= count;
    (inode->u.pipe_i.lock)--;
    wake_up_interruptible(&(inode->u.pipe_i.q.wait));
    if (count)
	inode->i_atime = CURRENT_TIME;
    else if ((inode->u.pipe_i.writers))
	count = (size_t)(-EAGAIN);
    return count;
}

static size_t pipe_write(register struct inode *inode, struct file *filp,
		      char *buf, size_t count)
{
    size_t free, end, written = 0;
    register char *chars;

    debug("PIPE: write called.\n");
    if (!(inode->u.pipe_i.readers)) {
	goto snd_signal;
    }

    free = (count <= PIPE_BUF) ? count : 1;
    while (count > 0) {
	while (((PIPE_BUF - (inode->u.pipe_i.q.len)) < free)
	       || (inode->u.pipe_i.lock)) {
	    if (!(inode->u.pipe_i.readers)) {
	      snd_signal:
		send_sig(SIGPIPE, current, 0);
		return written ? (int) written : -EPIPE;
	    }
	    if (current->signal)
		return written ? (int) written : -ERESTARTSYS;
	    if (filp->f_flags & O_NONBLOCK)
		return written ? (int) written : -EAGAIN;
	    interruptible_sleep_on(&(inode->u.pipe_i.q.wait));
	}
	(inode->u.pipe_i.lock)++;
	while (count > 0 && (free = (PIPE_BUF - inode->u.pipe_i.q.len))) {

	    end = (inode->u.pipe_i.q.start + inode->u.pipe_i.q.len)&(PIPE_BUF-1);
	    chars = (char *)(PIPE_BUF - end);
	    if ((size_t)chars > count)
		chars = (char *) count;
	    if ((size_t)chars > free)
		chars = (char *)free;

	    memcpy_fromfs((inode->u.pipe_i.q.base + end), buf, (size_t)chars);
	    buf += (size_t)chars;
	    (inode->u.pipe_i.q.len) += (size_t)chars;
	    written += (size_t)chars;
	    count -= (size_t)chars;
	}
	(inode->u.pipe_i.lock)--;
	wake_up_interruptible(&(inode->u.pipe_i.q.wait));
	free = 1;
    }
    inode->i_ctime = inode->i_mtime = CURRENT_TIME;

    return written;
}

#ifdef STRICT_PIPES
static void pipe_read_release(register struct inode *inode, struct file *filp)
{
    debug("PIPE: read_release called.\n");
    (inode->u.pipe_i.readers)--;
    wake_up_interruptible(&(inode->u.pipe_i.q.wait));
}

static void pipe_write_release(register struct inode *inode, struct file *filp)
{
    debug("PIPE: write_release called.\n");
    (inode->u.pipe_i.writers)--;
    wake_up_interruptible(&(inode->u.pipe_i.q.wait));
}
#endif

static void pipe_rdwr_release(register struct inode *inode,
			      register struct file *filp)
{
    debug("PIPE: rdwr_release called.\n");

    if (filp->f_mode & FMODE_READ)
	(inode->u.pipe_i.readers)--;

    if (filp->f_mode & FMODE_WRITE)
	(inode->u.pipe_i.writers)--;

    if (!(inode->u.pipe_i.readers + inode->u.pipe_i.writers)) {
	if (inode->u.pipe_i.q.base) {
	/* Free up any memory allocated to the pipe */
	    free_pipe_mem(inode->u.pipe_i.q.base);
	    inode->u.pipe_i.q.base = NULL;
	}
    }
    else
	wake_up_interruptible(&(inode->u.pipe_i.q.wait));
}

#ifdef STRICT_PIPES
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
#endif

static int pipe_rdwr_open(register struct inode *inode,
			  register struct file *filp)
{
    debug("PIPE: rdwr called.\n");

    if (!PIPE_BASE(*inode)) {
	if (!(PIPE_BASE(*inode) = get_pipe_mem()))
	    return -ENOMEM;
#if 0
	inode->u.pipe_i.q.size = PAGE_SIZE;
	/* next fields already set to zero by get_empty_inode() */
	PIPE_START(*inode) = PIPE_LEN(*inode) = 0;
	PIPE_RD_OPENERS(*inode) = PIPE_WR_OPENERS(*inode) = 0;
	PIPE_READERS(*inode) = PIPE_WRITERS(*inode) = 0;
	PIPE_LOCK(*inode) = 0;
#endif
    }
    if (filp->f_mode & FMODE_READ) {
	(inode->u.pipe_i.readers)++;
	if (inode->u.pipe_i.writers > 0) {
	    if (inode->u.pipe_i.readers < 2)
		wake_up_interruptible(&(inode->u.pipe_i.q.wait));
	}
	else {
	    if (!(filp->f_flags & O_NONBLOCK) && (inode->i_sb))
		while (!(inode->u.pipe_i.writers))
		    interruptible_sleep_on(&(inode->u.pipe_i.q.wait));
	}
    }

    if (filp->f_mode & FMODE_WRITE) {
	(inode->u.pipe_i.writers)++;
	if (inode->u.pipe_i.readers > 0) {
	    if (inode->u.pipe_i.writers < 2)
		wake_up_interruptible(&(inode->u.pipe_i.q.wait));
	}
	else {
	    if (filp->f_flags & O_NONBLOCK)
		return -ENXIO;
	    while (!(inode->u.pipe_i.readers))
		interruptible_sleep_on(&(inode->u.pipe_i.q.wait));
	}
    }
    return 0;
}

#ifdef STRICT_PIPES
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
#endif

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

static int do_pipe(register int *fd)
{
    register struct inode *inode;
    struct file *f;
    int error = -ENOMEM;

    if (!(inode = new_inode(NULL, S_IFIFO | S_IRUSR | S_IWUSR)))	/* Create inode */
	goto no_inodes;

    /* read file */
    if ((error = open_fd(O_RDONLY, inode)) < 0)
	goto no_files;

    *fd = error;

    /* write file */
    if ((error = open_fd(O_WRONLY, inode)) < 0) {
	f = current->files.fd[*fd];
	current->files.fd[*fd] = NULL;
	close_filp(inode, f);
      no_files:
	iput(inode);
      no_inodes:
	return error;
    }
    (inode->i_count)++;		/* Increase inode usage count */
    fd[1] = error;

    return 0;
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

#endif
