#ifndef _MALLOC_H
#define	_MALLOC_H

#include <sys/types.h>

typedef union mem_cell
{
   union mem_cell __wcnear *next;	/* A pointer to the next mem */
   unsigned int  size;		        /* An int >= sizeof pointer */
   char __wcnear *depth;		/* For the alloca hack */
} mem;

void __noise(char *y, mem __wcnear *x);
int __dprintf(const char *fmt, ...);
extern int __debug_level;

#if !VERBOSE
#define dprintf(...)
#define debug(str,ptr)
#elif VERBOSE == 1
#define dprintf             __dprintf
#define debug(str,ptr)
#else
#define dprintf(...)
#define debug(str,ptr)      __noise(str,ptr)
#endif

#define m_deep(p)  ((p) [0].depth)	/* For alloca */
#define m_next(p)  ((p) [1].next)	/* For malloc and alloca */
#define m_size(p)  ((p) [0].size)	/* For malloc */

extern mem __wcnear *__freed_list;

#endif
