#ifndef __MALLOC_H
#define __MALLOC_H

#include <sys/types.h>

void *malloc(size_t);
void free(void *);
void *realloc(void *, size_t);
void *calloc(size_t elm, size_t sz);

#ifdef __LIBC__
/*
 * Mini malloc allows you to use a less efficient but smaller malloc the
 * cost is about 100 bytes of code in free but malloc (700bytes) doesn't
 * have to be linked. Unfortunatly memory can only be reused if everything
 * above it has been freed
 * 
 */

/* remove __MINI_MALLOC__ and always use real malloc for libc routines */
//#define __MINI_MALLOC__

void __wcnear *__mini_malloc(size_t size);
#endif

#ifdef __MINI_MALLOC__
extern void __wcnear *(*__alloca_alloc)(size_t);
#define malloc(x) ((*__alloca_alloc)(x))
#endif

/* alloc/free from main memory */
void __far *fmemalloc(unsigned long size);
int fmemfree(void __far *ptr);
int _fmemalloc(int paras, unsigned short *pseg);
int _fmemfree(unsigned short seg);

#endif
