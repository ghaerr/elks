/*
 *  linux/fs/minix/file.c
 *
 *  Copyright (C) 1991, 1992 Linus Torvalds
 *
 *  minix regular file handling primitives
 */

#include <linuxmt/types.h>
#include <linuxmt/fs.h>
#include <linuxmt/minix_fs.h>
#include <linuxmt/stat.h>

/*
 * We have mostly NULL's here: the current defaults are ok for
 * the minix filesystem.
 */
static struct file_operations minix_file_operations = {
    NULL,                       /* lseek - default */
    block_read,                 /* read */
    block_write,                /* write */
    NULL,                       /* readdir - bad */
    NULL,                       /* select - default */
    NULL,                       /* ioctl - default */
    NULL,                       /* no special open is needed */
    NULL                        /* release */
};

struct inode_operations minix_file_inode_operations = {
    &minix_file_operations,     /* default file operations */
    NULL,                       /* create */
    NULL,                       /* lookup */
    NULL,                       /* link */
    NULL,                       /* unlink */
    NULL,                       /* symlink */
    NULL,                       /* mkdir */
    NULL,                       /* rmdir */
    NULL,                       /* mknod */
    NULL,                       /* readlink */
    NULL,                       /* follow_link */
    minix_getblk,               /* getblk */
    minix_truncate              /* truncate */
};
