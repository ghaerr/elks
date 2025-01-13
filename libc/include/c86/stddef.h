#ifndef __STDDEF_H
#define __STDDEF_H
/* stddef.h for C86 */

typedef unsigned int size_t;        /* must match STP_SIZE in C86 config.h */
typedef int          ptrdiff_t;     /* must match STP_PTRDIFF in C86 config.h */
typedef int          wchar_t;       /* doesn't currently match STP_WIDE in C86 config.h */

#define offsetof(__typ,__id) ((size_t)((char *)&(((__typ*)0)->__id) - (char *)0))

#endif
