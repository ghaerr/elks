#ifndef _MALLOC_H
#define _MALLOC_H

/*
 * Internal types for default malloc (dev86)
 */
#include <sys/types.h>

typedef union mem_cell
{
   union mem_cell __wcnear *next;       /* A pointer to the next mem */
   unsigned int  size;                  /* An int >= sizeof pointer */
   char __wcnear *depth;                /* For the alloca hack */
} mem;

void __wcnear *__mini_malloc(size_t size);
void __noise(char *y, mem __wcnear *x);
int __dprintf(const char *fmt, ...);
extern int __debug_level;
extern mem __wcnear *__freed_list;

#if !VERBOSE
#define dprintf(...)
#define debug(str,ptr)
#elif VERBOSE == 1
#define dprintf(...)  do { if (__debug_level) __dprintf(__VA_ARGS__); } while (0)
#define debug(str,ptr)
#else
#define dprintf(...)
#define debug(str,ptr)      __noise(str,ptr)
#endif

#define m_deep(p)  ((p) [0].depth)      /* For alloca */
#define m_next(p)  ((p) [1].next)       /* For malloc and alloca */
#define m_size(p)  ((p) [0].size)       /* For malloc */

/*
 * Mini malloc allows you to use a less efficient but smaller malloc the
 * cost is about 100 bytes of code in free but malloc (700bytes) doesn't
 * have to be linked. Unfortunatly memory can only be reused if everything
 * above it has been freed
 */
/* remove __MINI_MALLOC__ and always use real malloc for libc routines */
/*#define __MINI_MALLOC__*/
#ifdef __MINI_MALLOC__
extern void __wcnear    *(*__alloca_alloc)(size_t);
#define malloc(x)       ((*__alloca_alloc)(x))    /* NOTE won't work anymore */
#endif

#endif
