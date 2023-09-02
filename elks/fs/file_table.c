/*
 *  linux/fs/file_table.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/config.h>
#include <linuxmt/limits.h>
#include <linuxmt/fs.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/stat.h>
#include <linuxmt/string.h>
#include <linuxmt/mm.h>
#include <linuxmt/errno.h>

/*
 * first_file points to a doubly linked list of all file structures in
 *            the system.
 */

struct file file_array[NR_FILE];

/*
 * Find an unused file structure and return a pointer to it.
 * Returns NULL, if there are no more free file structures or
 * we run out of memory.
 */

int open_filp(unsigned short flags, struct inode *inode, struct file **fp)
{
    int result = 0;
    register struct file *f = file_array;
    register struct file_operations *fop;

    while (f->f_count) {
	if (++f >= &file_array[NR_FILE]) {
	    printk("open: No files\n");
	    return -ENFILE;
	}
    }
    memset(f, 0, sizeof(struct file));
    f->f_flags = flags;
    f->f_mode = (mode_t) ((flags + 1) & O_ACCMODE);
    f->f_count = 1;
/*  f->f_pos = 0;*/	/* FIXME - should call lseek */
    f->f_inode = inode;

#ifdef BLOAT_FS
    if (f->f_mode & FMODE_WRITE) {
	result = get_write_access(inode);
	if (result) goto cleanup_file;
    }
#endif

    if (inode->i_op) f->f_op = inode->i_op->default_file_ops;
    fop = f->f_op;
    if (fop && fop->open && (result = fop->open(inode, f))) {
#ifdef BLOAT_FS
	if (f->f_mode & FMODE_WRITE) put_write_access(inode);
      cleanup_file:
#endif
	f->f_count--;
    }
    *fp = f;

    return result;
}

void close_filp(struct inode *inode, register struct file *f)
{
    register struct file_operations *fop;

    fop = f->f_op;
    if (fop && fop->release) fop->release(inode, f);

#ifdef BLOAT_FS
    if (f->f_mode & FMODE_WRITE) put_write_access(inode);
#endif
    f->f_count--;
}
