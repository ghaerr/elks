/*
 *  linux/fs/file_table.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/types.h>
#include <linuxmt/config.h>
#include <linuxmt/fs.h>
#include <linuxmt/string.h>
#include <linuxmt/mm.h>

/*
 * first_file points to a doubly linked list of all file structures in
 *            the system.
 * nr_files   holds the length of this list.
 */

int nr_files = NR_FILE;
struct file file_array[NR_FILE];

/*
 * Find an unused file structure and return a pointer to it.
 * Returns NULL, if there are no more free file structures or
 * we run out of memory.
 */

struct file *get_empty_filp(void)
{
    int i;
    register struct file *f;

    for (f = file_array, i = 0; i < nr_files; i++, f++) {
	if (!f->f_count) {
	    memset(f, 0, sizeof(*f));
	    f->f_count = 1;
#ifdef BLOAT_FS
	    f->f_version = ++event;
#endif
	    return f;
	}
    }

    return NULL;
}
