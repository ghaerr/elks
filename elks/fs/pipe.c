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

int get_unused_fd()
{
        int fd;

        for(fd = 0; fd < NR_OPEN; fd++) {
                if (!current->files.fd[fd]) {
                        clear_bit(fd,&current->files.close_on_exec);
                        return fd;
                }
        }
        return -EMFILE;
}

#ifdef CONFIG_PIPE

char pipe_base[8][PIPE_BUF];
int pipe_in_use[8] = {0, };

char * get_pipe_mem()
{
	int i;

	for (i = 0; i < 8; i++) {
		if (!pipe_in_use[i]) {
			pipe_in_use[i] = 1;
			return pipe_base[i];
		}
	}
	printd_pipe("PIPE: No more buffers.\n"); /* FIXME */
	return NULL;
}


static int pipe_read(inode, filp, buf, count)
register struct inode * inode;
struct file * filp;
char * buf;
int count;
{
	int chars = 0, size = 0, read = 0;
	register char *pipebuf;

	printd_pipe("PIPE read called.\n");
	if (filp->f_flags & O_NONBLOCK) {
		if ((inode->u.pipe_i.lock) )
			return -EAGAIN;
		if (((inode->u.pipe_i.len)  ==0) )
			if ((inode->u.pipe_i.writers) )
				return -EAGAIN;
			else
				return 0;
	} else while (((inode->u.pipe_i.len)  ==0)  || (inode->u.pipe_i.lock) ) {
		if (((inode->u.pipe_i.len)  ==0) ) {
			if (! (inode->u.pipe_i.writers) )
				return 0;
		}
		if (current->signal )
			return -ERESTARTSYS;
		interruptible_sleep_on(& (inode->u.pipe_i.wait) );
	}
	(inode->u.pipe_i.lock) ++;
	while (count>0 && (size = (inode->u.pipe_i.len)  )) {
		chars = (PIPE_BUF - (inode->u.pipe_i.start) ) ;
		if (chars > count)
			chars = count;
		if (chars > size)
			chars = size;
		read += chars;
		pipebuf = (inode->u.pipe_i.base) + (inode->u.pipe_i.start) ;
		(inode->u.pipe_i.start)  += chars;
		(inode->u.pipe_i.start)  &= (PIPE_BUF-1);
		(inode->u.pipe_i.len)  -= chars;
		count -= chars;
		memcpy_tofs(buf, pipebuf, chars );
		buf += chars;
	}
	(inode->u.pipe_i.lock) --;
	wake_up_interruptible(& (inode->u.pipe_i.wait) );
	if (read) {
		inode->i_atime = CURRENT_TIME;
		return read;
	}
	if ((inode->u.pipe_i.writers) )
		return -EAGAIN;
	return 0;
}

static int pipe_write(inode, filp, buf, count)
register struct inode * inode;
struct file * filp;
char * buf;
int count;
{
	int chars = 0, free = 0, written = 0;
	register char *pipebuf;

	printd_pipe("PIPE write called.\n");
	if (! (inode->u.pipe_i.readers) ) {  
		send_sig(SIGPIPE,current,0);
		return -EPIPE;
	}
	 
	if (count <= PIPE_BUF)
		free = count;
	else
		free = 1;  
	while (count>0) {
		while (((PIPE_BUF - (inode->u.pipe_i.len) )  < free) || (inode->u.pipe_i.lock) ) {
			if (! (inode->u.pipe_i.readers) ) {  
				send_sig(SIGPIPE,current,0);
				return written? written :-EPIPE;
			}
			if (current->signal )
				return written? written :-ERESTARTSYS;
			if (filp->f_flags & O_NONBLOCK)
				return written? written :-EAGAIN;
			interruptible_sleep_on(& (inode->u.pipe_i.wait) );
		}
		(inode->u.pipe_i.lock) ++;
		while (count>0 && (free = (PIPE_BUF - (inode->u.pipe_i.len) ) )) {
			chars = (PIPE_BUF - (((inode->u.pipe_i.start) + (inode->u.pipe_i.len) )&	(PIPE_BUF-1)) ) ;
			if (chars > count)
				chars = count;
			if (chars > free)
				chars = free;
			pipebuf = (inode->u.pipe_i.base) + (((inode->u.pipe_i.start) + (inode->u.pipe_i.len) )&	(PIPE_BUF-1)) ;
			written += chars;
			(inode->u.pipe_i.len)  += chars;
			count -= chars;
			memcpy_fromfs(pipebuf, buf, chars );
			buf += chars;
		}
		(inode->u.pipe_i.lock) --;
		wake_up_interruptible(& (inode->u.pipe_i.wait) );
		free = 1;
	}
	inode->i_ctime = inode->i_mtime = CURRENT_TIME;
	return written;
}

static void pipe_read_release(inode, filp)
register struct inode * inode;
struct file * filp;
{
	printd_pipe("PIPE read_release called.\n");
        (inode->u.pipe_i.readers) --;
        wake_up_interruptible(& (inode->u.pipe_i.wait) );
}

static void pipe_write_release(inode, filp)
register struct inode * inode;
struct file * filp;
{
	printd_pipe("PIPE write_release called.\n");
	(inode->u.pipe_i.writers) --;
	wake_up_interruptible(& (inode->u.pipe_i.wait) );
}

static void pipe_rdwr_release(inode, filp)
register struct inode * inode;
register struct file * filp;
{
	printd_pipe("PIPE rdwr_release called.\n");
	if (filp->f_mode & FMODE_READ)
		(inode->u.pipe_i.readers) --;
	if (filp->f_mode & FMODE_WRITE)
		(inode->u.pipe_i.writers) --;
	wake_up_interruptible(& (inode->u.pipe_i.wait) );
}

static int pipe_read_open(inode, filp)
struct inode * inode;
struct file * filp;
{
	printd_pipe("PIPE read_open called.\n");
	(inode->u.pipe_i.readers) ++;
	return 0;
}

static int pipe_write_open(inode, filp)
struct inode * inode;
struct file * filp;
{
	printd_pipe("PIPE write_open called.\n");
        (inode->u.pipe_i.writers) ++;
        return 0;
}

static int pipe_rdwr_open(inode, filp)
register struct inode * inode;
register struct file * filp;
{
	printd_pipe("PIPE rdwr called.\n");
	if (filp->f_mode & FMODE_READ)
		(inode->u.pipe_i.readers) ++;
	if (filp->f_mode & FMODE_WRITE)
		(inode->u.pipe_i.writers) ++;
        return 0;
}


static int pipe_lseek(inode, file, offset, orig)
struct inode * inode;
struct file * file;
off_t offset;
int orig;
{
	printd_pipe("PIPE lseek called.\n");
	return -ESPIPE;
}

static int bad_pipe_rw(inode,filp,buf,count)
struct inode * inode;
struct file * filp;
char * buf;
int count;
{
	printd_pipe("PIPE bad rw called.\n");
	return -EBADF;
}

struct file_operations read_pipe_fops = {
        pipe_lseek,
        pipe_read,
        bad_pipe_rw,
        NULL,           /* no readdir */
        NULL,		/* select */
        NULL,		/* ioctl */
        pipe_read_open,
        pipe_read_release,
};

struct file_operations write_pipe_fops = {
        pipe_lseek,
        bad_pipe_rw,
        pipe_write,
        NULL,           /* no readdir */
        NULL,		/* select */
        NULL,		/* ioctl */
        pipe_write_open,
        pipe_write_release,
};

struct file_operations rdwr_pipe_fops = {
        pipe_lseek,
        pipe_read,
        pipe_write,
        NULL,           /* no readdir */
        NULL,		/* select */
        NULL,		/* ioctl */
        pipe_rdwr_open,
        pipe_rdwr_release,
};


struct inode_operations pipe_inode_operations = {
	&rdwr_pipe_fops,
	NULL,                   /* create */
	NULL,                   /* lookup */
	NULL,                   /* link */
	NULL,                   /* unlink */
	NULL,                   /* symlink */
	NULL,                   /* mkdir */
	NULL,                   /* rmdir */
	NULL,                   /* mknod */
	NULL,                   /* readlink */
	NULL,                   /* follow_link */
#ifdef BLOAT_FS
	NULL,                   /* bmap */
#endif
	NULL,                   /* truncate */
#ifdef BLOAT_FS
	NULL                    /* permission */
#endif
};


int do_pipe(fd)
int * fd;
{
	struct inode * inode;
	register struct file * f1;
	register struct file * f2;
	int error;
	int i,j;

        error = ENFILE;
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
/*        put_unused_fd(i); Not sure this is needed */
        current->files.fd[i] = NULL;
close_f12_inode:
        inode->i_count--;
        iput(inode);
close_f12:
        f2->f_count--;
close_f1:
        f1->f_count--;
no_files:
        return error;   

}


int sys_pipe(filedes)
unsigned int * filedes;
{
	int fd[2];
	int error;

	printd_pipe("PIPE called.\n");
	if (error = do_pipe(fd))
		return error;
	printd2_pipe("PIPE worked %d %d.\n", fd[0], fd[1]);

	return verified_memcpy_tofs(filedes, fd, 2 * sizeof(int));
}
#else
int sys_pipe(filedes)
{
	return -ENOSYS;
}

#endif /* CONFIG_PIPE */
