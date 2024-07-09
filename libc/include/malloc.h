#ifndef __MALLOC_H
#define __MALLOC_H

#include <sys/types.h>

/*
 * Mini malloc allows you to use a less efficient but smaller malloc the
 * cost is about 100 bytes of code in free but malloc (700bytes) doesn't
 * have to be linked. Unfortunatly memory can only be reused if everything
 * above it has been freed
 * 
 */

void free(void *);
void *calloc(unsigned int elm, unsigned int sz);
void *malloc(size_t);
void *realloc(void *, size_t);
void *alloca(size_t);

#ifdef __LIBC__
/* remove __MINI_MALLOC__ and always use real malloc for libc routines*/
//#define __MINI_MALLOC__

void __wcnear *__mini_malloc(size_t size);
#endif

#ifdef __MINI_MALLOC__
extern void __wcnear *(*__alloca_alloc)(size_t);
#define malloc(x) ((*__alloca_alloc)(x))
#endif

/* alloc from main memory */
void __far *fmemalloc(unsigned long size);
int _fmemalloc(int paras, seg_t *pseg);

#endif
