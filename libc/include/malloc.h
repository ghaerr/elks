#ifndef __MALLOC_H
#define __MALLOC_H

#include <sys/types.h>

/* default malloc (dev86) */
void   *malloc(size_t);
void   *realloc(void *, size_t);
void    free(void *);
size_t  malloc_usable_size(void *);

/* debug malloc (v7 malloc) */
void   *__dmalloc(size_t);
void   *__drealloc(void *, size_t);
void    __dfree(void *);
size_t  __dmalloc_usable_size(void *);

/* arena malloc (64k near/unlimited far heap) */
void   *__amalloc(size_t);
int     __amalloc_add_heap(char __far *start, size_t size);
void   *__arealloc(void *, size_t);         /* not implemented */
void    __afree(void *);
size_t  __amalloc_usable_size(void *);
extern unsigned int malloc_arena_size;
extern unsigned int malloc_arena_thresh;

/* usable with all mallocs */
void   *calloc(size_t elm, size_t sz);

/* alloc/free from main memory */
void __far *fmemalloc(unsigned long size);
int         fmemfree(void __far *ptr);

int        _fmemalloc(int paras, unsigned short *pseg); /* syscall */
int        _fmemfree(unsigned short seg);               /* syscall */

/* debug output */
int __dprintf(const char *fmt, ...);

#endif
