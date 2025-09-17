#ifndef __MALLOC_H
#define __MALLOC_H

#include <sys/types.h>

/* default malloc (dev86 near heap) */
void   *malloc(size_t);
void   *realloc(void *, size_t);
void    free(void *);
size_t  malloc_usable_size(void *);

/* debug malloc (v7 malloc near heap) */
void   *_dmalloc(size_t);
void   *_drealloc(void *, size_t);
void    _dfree(void *);
size_t  _dmalloc_usable_size(void *);

#ifndef __STRICT_ANSI__
/* _fmalloc (single arena 64k far heap) */
void __far *_fmalloc(size_t);
int         _fmalloc_add_heap(char __far *start, size_t size);
void __far *_frealloc(void *, size_t);         /* not implemented */
void        _ffree(void __far *);
size_t      _fmalloc_usable_size(void __far *);
extern unsigned int malloc_arena_size;          /* mem.c wrapper function */
extern unsigned int malloc_arena_thresh;        /* mem.c wrapper function */

/* alloc/free from main memory */
void __far *fmemalloc(unsigned long size);
int         fmemfree(void __far *ptr);

int        _fmemalloc(int paras, unsigned short *pseg); /* syscall */
int        _fmemfree(unsigned short seg);               /* syscall */
#endif

/* usable with all mallocs */
void   *calloc(size_t elm, size_t sz);

/* debug output */
int __dprintf(const char *fmt, ...);

#endif
