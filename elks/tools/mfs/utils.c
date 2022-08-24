/*
 * Copyright (C) 2005 - Alejandro Liu Ly <alejandro_liu@hotmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * @project	mfstool
 * @module	utils
 * @section	3
 * @doc	Miscellaneous utilities
 */
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "minix_fs.h"
#include "protos.h"

/**
 * Print an error message and die.
 * @param s - string format
 * @effect This function causes the program to die
 */
void fatalmsg(const char *s,...) {
  va_list p;
  va_start(p,s);
  vfprintf(stderr,s,p);
  va_end(p);
  putc('\n',stderr);  
  exit(-1);
}

/**
 * Like fatalmsg but also show the errno message.
 * @param s - string format
 * @effect This function causes the program to die.
 */
void die(const char *s,...) {
  va_list p;
  va_start(p,s);
  vfprintf(stderr,s,p);
  va_end(p);
  putc(':',stderr);
  putc(' ',stderr);
  perror(NULL);
  putc('\n',stderr);
  exit(errno);
}

/**
 * Seek file to the specified block
 * @param fp - Pointer to the filesystem file.
 * @param blk - Block to go to
 * @return fp
 */
FILE *goto_blk(FILE *fp,unsigned int blk) {
  fflush(fp);
  if (fseek(fp,blk*BLOCK_SIZE,SEEK_SET)) {
    die("fseek");
  }
  return fp;
}

/**
 * Write cnt bytes to file.
 * @param fp - Pointer to filesystem file.
 * @param buff - data to write
 * @param cnt - bytes to write
 * @return buff
 */
void *dofwrite(FILE *fp,void *buff,int cnt) {
  if (cnt != fwrite(buff,1,cnt,fp)) {
    die("fwrite");
  }
  return buff;
}

/**
 * Read cnt bytes from file
 * @param fp - Pointer to filesystem file.
 * @param buff - Buffer (BLOCK_SIZE) big.
 * @param cnt - bytes to read
 * @return buff
 */
void *dofread(FILE *fp,void *buff,int cnt) {
  if (cnt != fread(buff,1,cnt,fp)) {
    die("fread");
  }
  return buff;
}

/**
 * Allocate memory (w/error handling) and memory clearing.
 * @param size - number of bytes to allocate
 * @param elm - Initialize memory to this value.  -1 for no initialization
 * @return pointer to newly allocated memory.
 */
void *domalloc(unsigned long size,int elm) {
  void *ptr = malloc(size);
  if (!ptr) {
    die("malloc");
  }
  if (elm >= 0) {
    memset(ptr,elm,size);
  }
  return ptr;
}

/**
 * Wrapper around getuid
 */
int dogetuid(void) {
  if (opt_keepuid)
    return getuid();    
  return 0;
}

/**
 * Wrapper around getgid
 */
int dogetgid(void) {
  if (opt_keepuid)
  	return getgid();    
  return 0;
}
