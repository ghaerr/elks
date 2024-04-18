/*
 *  elks/fs/pipe.c
 *
 * Copyright (C) 1991, 1992 Linux Torvalds
 *               1998 Alistair Riddoch
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
#include <linuxmt/mm.h>
#include <linuxmt/fs.h>
#include <linuxmt/kdev_t.h>
#include <linuxmt/wait.h>
#include <linuxmt/heap.h>
#include <linuxmt/debug.h>

#include <arch/system.h>
#include <arch/segment.h>
#include <arch/bitops.h>

static int get_unused_fd(struct file *f)
{
    int fd = 0;
    register struct file **cfs = current->files.fd;

    do {
        if (!*cfs) {
            *cfs = f;
            clear_bit(fd, &(current->files.close_on_exec));
            return fd;
        }
        cfs++;
    } while (++fd < NR_OPEN);
    return -EMFILE;
}

int open_fd(int flags, struct inode *inode)
{
    int fd;
    struct file *f;
    register struct file *filp;

    if (!(fd = open_filp(flags, inode, &f))) {
        filp = f;
        filp->f_flags &= ~(O_CREAT | O_EXCL | O_NOCTTY | O_TRUNC);
    /*
     * We have to do this last, because we mustn't export
     * an incomplete fd to other processes which may share
     * the same file table with us.
     */
        if ((fd = get_unused_fd(filp)) < 0)
            close_filp(inode, filp);
    }
    return fd;
}

int pipe_lseek(struct inode *inode, struct file *file, loff_t offset,
                  int orig)
{
    debug("PIPE: lseek called.\n");
    return -ESPIPE;
}

/* pipes are allocated from kernel local heap */
static unsigned char *get_pipe_mem(void)
{
    return heap_alloc(PIPE_BUFSIZ, HEAP_TAG_PIPE);
}

static void free_pipe_mem(unsigned char *buf)
{
    heap_free(buf);
}

static size_t pipe_read(register struct inode *inode, struct file *filp,
                     char *buf, size_t count)
{
    size_t chars;

    debug("PIPE: read called.\n");
    while (PIPE_EMPTY(inode) || PIPE_LOCK(inode)) {
        if (!PIPE_LOCK(inode) && !PIPE_WRITERS(inode)) return 0;
        if (filp->f_flags & O_NONBLOCK) return -EAGAIN;
        if (current->signal) return -ERESTARTSYS;       // FIXME
        interruptible_sleep_on(&PIPE_WAIT(inode));
    }
    PIPE_LOCK(inode)++;
    if (count > PIPE_LEN(inode)) count = PIPE_LEN(inode);
    chars = PIPE_SIZE(inode) - PIPE_TAIL(inode);
    if (chars > count) chars = count;
    memcpy_tofs(buf, PIPE_BASE(inode) + PIPE_TAIL(inode), chars);
    if (chars < count)
        memcpy_tofs(buf + chars, PIPE_BASE(inode), count - chars);
    if ((PIPE_TAIL(inode) += count) >= PIPE_SIZE(inode))
        PIPE_TAIL(inode) -= PIPE_SIZE(inode);
    PIPE_LEN(inode) -= count;
    PIPE_LOCK(inode)--;
    wake_up_interruptible(&PIPE_WAIT(inode));
    if (count) inode->i_atime = current_time();
    else if (PIPE_WRITERS(inode)) count = -EAGAIN;
    return count;
}

static size_t pipe_write(register struct inode *inode, struct file *filp,
                      char *buf, size_t count)
{
    size_t free, head, chars, written = 0;

    debug("PIPE: write called.\n");
    if (!PIPE_READERS(inode)) goto snd_signal;

    free = (count <= PIPE_SIZE(inode)) ? count : 1;
    while (count > 0) {
        while (((PIPE_SIZE(inode) - PIPE_LEN(inode)) < free) || PIPE_LOCK(inode)) {
            if (!PIPE_READERS(inode)) {
              snd_signal:
                send_sig(SIGPIPE, current, 0);
                return written ? written : -EPIPE;
            }
            if (current->signal) return written ? written : -ERESTARTSYS; // FIXME
            if (filp->f_flags & O_NONBLOCK)
                return written ? written : -EAGAIN;
            interruptible_sleep_on(&PIPE_WAIT(inode));
        }
        PIPE_LOCK(inode)++;
        while (count > 0 && (free = (PIPE_SIZE(inode) - PIPE_LEN(inode)))) {
            head = PIPE_HEAD(inode);
            chars = PIPE_SIZE(inode) - head;
            if (chars > count) chars = count;
            if (chars > free) chars = free;

            memcpy_fromfs(PIPE_BASE(inode) + head, buf, chars);
            buf += chars;
            if ((PIPE_HEAD(inode) += chars) >= PIPE_SIZE(inode))
                PIPE_HEAD(inode) -= PIPE_SIZE(inode);
            PIPE_LEN(inode) += chars;
            written += chars;
            count -= chars;
        }
        PIPE_LOCK(inode)--;
        wake_up_interruptible(&PIPE_WAIT(inode));
        free = 1;
    }
    inode->i_ctime = inode->i_mtime = current_time();

    return written;
}

#ifdef STRICT_PIPES
static void pipe_read_release(register struct inode *inode, struct file *filp)
{
    debug("PIPE: read_release called.\n");
    PIPE_READERS(inode)--;
    wake_up_interruptible(&PIPE_WAIT(inode));
}

static void pipe_write_release(register struct inode *inode, struct file *filp)
{
    debug("PIPE: write_release called.\n");
    PIPE_WRITERS(inode)--;
    wake_up_interruptible(&PIPE_WAIT(inode));
}
#endif

static void pipe_rdwr_release(register struct inode *inode,
                              register struct file *filp)
{
    debug("PIPE: rdwr_release called.\n");

    if (filp->f_mode & FMODE_READ) PIPE_READERS(inode)--;
    if (filp->f_mode & FMODE_WRITE) PIPE_WRITERS(inode)--;

    if (!(PIPE_READERS(inode) + PIPE_WRITERS(inode))) {
        if (PIPE_BASE(inode)) {
            /* Free up any memory allocated to the pipe */
            free_pipe_mem(PIPE_BASE(inode));
            PIPE_BASE(inode) = NULL;
        }
    } else wake_up_interruptible(&PIPE_WAIT(inode));
}

#ifdef STRICT_PIPES
static int pipe_read_open(struct inode *inode, struct file *filp)
{
    debug("PIPE: read_open called.\n");
    PIPE_READERS(inode)++;

    return 0;
}

static int pipe_write_open(struct inode *inode, struct file *filp)
{
    debug("PIPE: write_open called.\n");
    PIPE_WRITERS(inode)++;

    return 0;
}
#endif

static int pipe_rdwr_open(register struct inode *inode,
                          register struct file *filp)
{
    debug("PIPE: rdwr called.\n");

    if (!PIPE_BASE(inode)) {
        if (!(PIPE_BASE(inode) = get_pipe_mem())) return -ENOMEM;
        /* PIPE_ fields set to zero by new_inode() */
        PIPE_SIZE(inode) = PIPE_BUFSIZ;
    }
    if (filp->f_mode & FMODE_READ) {
        PIPE_READERS(inode)++;
        if (PIPE_WRITERS(inode) > 0) {
            if (PIPE_READERS(inode) < 2)
                wake_up_interruptible(&PIPE_WAIT(inode));
        }
        else {
            if (!(filp->f_flags & O_NONBLOCK) && (inode->i_sb))
                while (!PIPE_WRITERS(inode))
                    interruptible_sleep_on(&PIPE_WAIT(inode));
        }
    }

    if (filp->f_mode & FMODE_WRITE) {
        PIPE_WRITERS(inode)++;
        if (PIPE_READERS(inode) > 0) {
            if (PIPE_WRITERS(inode) < 2)
                wake_up_interruptible(&PIPE_WAIT(inode));
        } else {
            if (filp->f_flags & O_NONBLOCK) return -ENXIO;
            while (!PIPE_READERS(inode))
                interruptible_sleep_on(&PIPE_WAIT(inode));
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

struct file_operations read_pipe_fops = {
    pipe_lseek, pipe_read, bad_pipe_rw, NULL,   /* no readdir */
    NULL,                       /* select */
    NULL,                       /* ioctl */
    pipe_read_open, pipe_read_release,
};

struct file_operations write_pipe_fops = {
    pipe_lseek,
    bad_pipe_rw,
    pipe_write,
    NULL,                       /* no readdir */
    NULL,                       /* select */
    NULL,                       /* ioctl */
    pipe_write_open,
    pipe_write_release,
};
#endif

struct file_operations rdwr_pipe_fops = {
    pipe_lseek,
    pipe_read,
    pipe_write,
    NULL,                       /* no readdir */
    NULL,                       /* select */
    NULL,                       /* ioctl */
    pipe_rdwr_open,
    pipe_rdwr_release,
};

struct inode_operations pipe_inode_operations = {
    &rdwr_pipe_fops, NULL,      /* create */
    NULL,                       /* lookup */
    NULL,                       /* link */
    NULL,                       /* unlink */
    NULL,                       /* symlink */
    NULL,                       /* mkdir */
    NULL,                       /* rmdir */
    NULL,                       /* mknod */
    NULL,                       /* readlink */
    NULL,                       /* follow_link */
    NULL,                       /* getblk */
    NULL                        /* truncate */
};

static int do_pipe(register int *fd)
{
    register struct inode *inode;
    int error = -ENOMEM;

    if (!(inode = new_inode(NULL, S_IFIFO | S_IRUSR | S_IWUSR)))        /* Create inode */
        goto no_inodes;

    /* read file */
    if ((error = open_fd(O_RDONLY, inode)) < 0) {
        iput(inode);
        goto no_inodes;
    }

    *fd = error;

    /* write file */
    if ((error = open_fd(O_WRONLY, inode)) < 0) {
        sys_close(*fd);
      no_inodes:
        return error;
    }
    (inode->i_count)++;         /* Increase inode usage count */
    fd[1] = error;

    return 0;
}

int sys_pipe(unsigned int *filedes)
{
    int fd[2];
    int error;

    debug("PIPE: called.\n");

    if ((error = do_pipe(fd))) return error;

    debug("PIPE: Returned %d %d.\n", fd[0], fd[1]);

    return verified_memcpy_tofs(filedes, fd, 2 * sizeof(int));
}
