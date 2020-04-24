#ifndef __MALLOC_H
#define __MALLOC_H
#include <features.h>
#include <sys/types.h>

/*
 * Mini malloc allows you to use a less efficient but smaller malloc the
 * cost is about 100 bytes of code in free but malloc (700bytes) doesn't
 * have to be linked. Unfortunatly memory can only be reused if everything
 * above it has been freed
 * 
 */

extern void free __P((void *));
extern void *calloc(unsigned int elm, unsigned int sz);
extern void *malloc __P((size_t));
extern void *realloc __P((void *, size_t));
extern void *alloca __P((size_t));

#ifdef __LIBC__
/* remove __MINI_MALLOC__ and always use real malloc for libc routines*/
//#define __MINI_MALLOC__

extern void *__mini_malloc(size_t size);
#endif

#ifdef __MINI_MALLOC__
extern void *(*__alloca_alloc) __P((size_t));
#define malloc(x) ((*__alloca_alloc)(x))
#endif

#endif
