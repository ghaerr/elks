/*
 *  linux/fs/file_table.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/types.h>
#include <linuxmt/config.h>
#include <linuxmt/fs.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/string.h>
#include <linuxmt/mm.h>

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

struct file *get_empty_filp(unsigned short flags)
{
    register struct file *f = file_array;

    do {
	if (!f->f_count) {	/* TODO: is nr_file const? */
	    memset(f, 0, sizeof(struct file));
	    f->f_flags = flags;
	    f->f_mode = (mode_t) ((flags + 1) & O_ACCMODE);
	    f->f_count = 1;
	    f->f_pos = 0;	/* FIXME - should call lseek */
#ifdef BLOAT_FS
	    f->f_version = ++event;
#endif
	    return f;
	}
    } while(++f < &file_array[NR_FILE]);

    return NULL;
}
