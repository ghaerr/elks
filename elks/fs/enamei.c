/*
 *   elks/fs/enamei.c
 *
 *   namei code for a kernel with no real filesystem
 *
 *   Copyright (C) 1999 Alistair Riddoch
 *
 *   based on code from namei.c
 *   Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linuxmt/fs.h>
#include <linuxmt/errno.h>

int open_namei(pathname,flag,mode,res_inode,base)
char * pathname;
int flag;
int mode;
register struct inode ** res_inode;
struct inode * base;
{
	return -ENOSYS;
}
