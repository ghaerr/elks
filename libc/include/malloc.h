#ifndef __MALLOC_H
#define __MALLOC_H

#include <sys/types.h>

/* default malloc (dev86) */
void *malloc(size_t);
void *realloc(void *, size_t);
void  free(void *);

/* debug malloc (v7 malloc) */
void *__dmalloc(size_t);
void *__drealloc(void *, size_t);
void  __dfree(void *);

/* arena malloc (64k near/unlimited far heap) */
void *__amalloc(size_t);
void *__arealloc(void *, size_t);
void  __afree(void *);

void *calloc(size_t elm, size_t sz);

/* alloc/free from main memory */
void __far *fmemalloc(unsigned long size);
int fmemfree(void __far *ptr);
int _fmemalloc(int paras, unsigned short *pseg);
int _fmemfree(unsigned short seg);

#endif
