/*
 *  linux/fs/open.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/stat.h>
#include <linuxmt/string.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/signal.h>
#include <linuxmt/time.h>
#include <linuxmt/fs.h>
#include <linuxmt/mm.h>
#include <linuxmt/utime.h>
#include <linuxmt/debug.h>

#include <arch/segment.h>
#include <arch/system.h>

int sys_utime(char *filename, register struct utimbuf *times)
{
    struct inode *inode;
    int error;

    error = namei(filename, &inode, 0, 0);
    if (!error) {
        if (times) {
            inode->i_atime = get_user_long((void *) &times->actime);
            inode->i_mtime = get_user_long((void *) &times->modtime);
        } else inode->i_atime = inode->i_mtime = current_time();
        inode->i_dirt = 1;
        iput(inode);
    }
    return error;
}

/*
 * access() needs to use the real uid/gid, not the effective uid/gid.
 * We do this by temporarily setting fsuid/fsgid to the wanted values
 */

int sys_access(char *filename, mode_t mode)
{
    struct inode *inode;
    uid_t old_euid;
    gid_t old_egid;
    int error;

    if (mode != (mode & S_IRWXO))       /* where's F_OK, X_OK, W_OK, R_OK? */
        error = -EINVAL;
    else {
        old_euid = current->euid;
        old_egid = current->egid;
        current->euid = current->uid;
        current->egid = current->gid;
        error = namei(filename, &inode, 0, mode);
        if (!error) iput(inode);
        current->euid = old_euid;
        current->egid = old_egid;
    }
    return error;
}

int sys_chdir(char *filename)
{
    struct inode *inode;
    int error;

    error = namei(filename, &inode, IS_DIR, MAY_EXEC);
    if (!error) {
        iput(current->fs.pwd);
        current->fs.pwd = inode;
    }
    return error;
}

int sys_chroot(char *filename)
{
    struct inode *inode;
    int error;

    error = (suser() ? namei(filename, &inode, IS_DIR, 0) : -EPERM);
    if (!error) {
        iput(current->fs.root);
        current->fs.root = inode;
    }
    return error;
}

int sys_chmod(char *filename, mode_t mode)
{
    struct inode *inode;
    register struct inode *inodep;
    int error = namei(filename, &inode, 0, 0);

#ifdef USE_NOTIFY_CHANGE
    struct iattr newattrs;
    register struct iattr *nap = &newattrs;

    inodep = inode;
    if (error) return error;
    if (IS_RDONLY(inodep)) {
        iput(inodep);
        return -EROFS;
    }
    if (mode == (mode_t) -1)
        mode = inodep->i_mode;
    nap->ia_mode = (mode & S_IALLUGO) | (inodep->i_mode & ~S_IALLUGO);
    nap->ia_valid = ATTR_MODE | ATTR_CTIME;
    inodep->i_dirt = 1;
    error = notify_change(inodep, nap);
    iput(inodep);
#else
    if (!error) {
        inodep = inode;
        if (IS_RDONLY(inodep)) {
            error = -EROFS;
            goto echmod;
        }
        if (mode == (mode_t) -1) mode = inodep->i_mode;
        mode = (mode & S_IALLUGO) | (inodep->i_mode & ~S_IALLUGO);
        if ((current->euid != inodep->i_uid) && !suser()) {
            error = -EPERM;
            goto echmod;
        }
        if (!suser() && !in_group_p(inodep->i_gid)) {
            mode &= ~S_ISGID;
        }
        inodep->i_mode = mode;
        inodep->i_dirt = 1;
      echmod:
        iput(inodep);
    }
#endif
    return error;
}

#ifdef USE_NOTIFY_CHANGE
static int do_chown(register struct inode *inode, uid_t user, gid_t group)
{
    struct iattr newattrs;
    register struct iattr *nap = &newattrs;

    if (IS_RDONLY(inode)) return -EROFS;
    if (user == (uid_t) - 1) user = inode->i_uid;
    if (group == (gid_t) - 1) group = inode->i_gid;
    nap->ia_mode = inode->i_mode;
    nap->ia_uid = user;
    nap->ia_gid = group;
    nap->ia_valid = ATTR_UID | ATTR_GID | ATTR_CTIME;

    /*
     * If the owner has been changed, remove the setuid bit
     */
    if (user != inode->i_uid && (inode->i_mode & S_ISUID)) {
        nap->ia_mode &= ~S_ISUID;
        nap->ia_valid |= ATTR_MODE;
    }

    /*
     * If the group has been changed, remove the setgid bit
     */
    if (group != inode->i_gid && (inode->i_mode & S_ISGID)) {
        nap->ia_mode &= ~S_ISGID;
        nap->ia_valid |= ATTR_MODE;
    }

    inode->i_dirt = 1;
    return notify_change(inode, nap);
}
#endif

static int do_chown(register struct inode *inode, uid_t user, gid_t group)
{
    if (IS_RDONLY(inode)) return -EROFS;

    if (!suser() && !(current->euid == inode->i_uid)) return -EPERM;

    if (group != (gid_t) -1 && inode->i_gid != group) {
        inode->i_gid = (__u8) group;
        inode->i_mode &= ~S_ISGID;
        inode->i_dirt = 1;
    }
    if (user != (uid_t) -1 && inode->i_uid != user) {
        inode->i_uid = user;
        inode->i_mode &= ~S_ISUID;
        inode->i_dirt = 1;
    }

    return 0;
}

int sys_chown(char *filename, uid_t user, gid_t group)
{
    struct inode *inode;
    int error;

    error = lnamei(filename, &inode);
    if (!error) {
        error = do_chown(inode, user, group);
        iput(inode);
    }
    return error;
}

int sys_fchown(unsigned int fd, uid_t user, gid_t group)
{
    register struct file *filp;

    return ((fd >= NR_OPEN || !(filp = current->files.fd[fd]) || !(filp->f_inode))
            ? -EBADF
            : do_chown(filp->f_inode, user, group));
}

/*
 * Note that while the flag value (low two bits) for sys_open means:
 *      00 - read-only
 *      01 - write-only
 *      10 - read-write
 *      11 - special
 * it is changed into
 *      00 - no permissions needed
 *      01 - read-permission
 *      10 - write-permission
 *      11 - read-write
 * for the internal routines (ie open_namei()/follow_link() etc). 00 is
 * used by symlinks.
 */

int sys_open(const char *filename, int flags, mode_t mode)
{
    struct inode *inode;
    int error, flag;

    flag = flags;
    if ((flags + 1) & O_ACCMODE) flag++;
    if (flag & (O_TRUNC | O_CREAT)) flag |= FMODE_WRITE;

    debug_file("OPEN '%t' flags %#x", filename, flags);
    error = open_namei(filename, flag, mode, &inode, NULL);
    if (!error) {
        if ((error = open_fd(flags, inode)) < 0)
            iput(inode);
    }
    debug_file(" = %d\n", error);
    return error;
}

static void close_fp(register struct file *filp)
{
    register struct inode *inode;

    if (!(filp->f_count)) printk("VFS: Close: file count is 0\n");
    else if (filp->f_count > 1) filp->f_count--;
    else {
        inode = filp->f_inode;
        close_filp(inode, filp);
        filp->f_inode = NULL;
        iput(inode);
    }
}

/* This is used by exit and sometimes exec to close all files of a process */

void _close_allfiles(void)
{
    register struct file **filp;
    int i = 0;

    filp = current->files.fd;
    do {
        if (*filp != NULL) {
            close_fp(*filp);
            *filp = NULL;
        }
        filp++;
    } while (++i < NR_OPEN);
}

int sys_close(unsigned int fd)
{
    register struct file *filp;
    register struct file_struct *cfiles = &current->files;

    debug_file("CLOSE %d\n", fd);
    if (fd < NR_OPEN) {
        clear_bit(fd, &cfiles->close_on_exec);
        if ((filp = cfiles->fd[fd])) {
            cfiles->fd[fd] = NULL;
            close_fp(filp);
            return 0;
        }
    }
    return -EBADF;
}

#if UNUSED
int sys_statfs(char *path, register struct statfs *ubuf)
{
    struct super_block *s;
    struct inode *inode;
    int error;
    struct statfs sbuf;

    error = namei(path, &inode, 0, 0);
    if (error)
        return error;
    if (!inode->i_sb->s_op->statfs_kern) {
        iput(inode);
        return -ENOSYS;
    }
    s = inode->i_sb;
    s->s_op->statfs_kern(inode->i_sb, &sbuf, 0);
    sbuf.f_type = s->s_type->type;
    sbuf.f_flags = s->s_flags;
    sbuf.f_dev = s->s_dev;
    memcpy(sbuf.f_mntonname, s->s_mntonname, MNAMELEN);

    iput(inode);

    return verified_memcpy_tofs(ubuf, &sbuf, sizeof(sbuf));
}

int do_truncate(register struct inode *inode, loff_t length)
{
    int error;
    struct iattr newattrs;
    register struct inode_operations *iop = inode->i_op;

    down(&inode->i_sem);
    newattrs.ia_size = length;
    newattrs.ia_valid = ATTR_SIZE | ATTR_CTIME;
    error = notify_change(inode, &newattrs);
    if (!error)
        if (iop && iop->truncate)
            iop->truncate(inode);
    up(&inode->i_sem);
    return error;
}

int sys_truncate(char *path, loff_t length)
{
    struct inode *inode;
    register struct inode *inodep;
    int error;

    error = namei(path, &inode, NOT_DIR, MAY_WRITE);
    inodep = inode;
    if (error)
        return error;
    if (IS_RDONLY(inodep)) {
        iput(inodep);
        return -EROFS;
    }
    error = do_truncate(inodep, length);
    iput(inodep);
    return error;
}

int sys_ftruncate(unsigned int fd, loff_t length)
{
    register struct inode *inode;
    register struct file *file;

    if (fd >= NR_OPEN || !(file = current->files.fd[fd]))
        return -EBADF;
    if (!(inode = file->f_inode))
        return -ENOENT;
    if (S_ISDIR(inode->i_mode) || !(file->f_mode & FMODE_WRITE))
        return -EACCES;
    return do_truncate(inode, length);
}

/* If times==NULL, set access and modification to current time,
 * must be owner or have write permission.
 * Else, update from *times, must be owner or super user.
 */

int sys_utimes(char *filename, struct timeval *utimes)
{
    int error;
    struct inode *inode;
    register struct inode *inodep;
    struct iattr newattrs;

    error = namei(filename, &inode, 0, 0);
    inodep = inode;
    if (error)
        return error;
    if (IS_RDONLY(inodep)) {
        iput(inodep);
        return -EROFS;
    }
    /* Don't worry, the checks are done in inode_change_ok() */
    newattrs.ia_valid = ATTR_CTIME | ATTR_MTIME | ATTR_ATIME;
    if (utimes) {
        struct timeval times[2];
        if (error = verified_memcpy_fromfs(&times, utimes, sizeof(times))) {
            iput(inodep);
            return error;
        }
        newattrs.ia_atime = times[0].tv_sec;
        newattrs.ia_mtime = times[1].tv_sec;
        newattrs.ia_valid |= ATTR_ATIME_SET | ATTR_MTIME_SET;
    } else if ((error = permission(inodep, MAY_WRITE)) != 0) {
        iput(inodep);
        return error;
    }
    error = notify_change(inodep, &newattrs);
    iput(inodep);
    return error;
}
#endif
