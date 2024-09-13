#ifndef __LIMITS_H
#define __LIMITS_H

#include <features.h>
#include __SYSINC__(limits.h)

/* Maximum number of bytes in a pathname, including the terminating null. */
#define PATH_MAX        128

/* Maximum number of bytes in a filename, not including terminating null. */
#define NAME_MAX        MAXNAMLEN

#define PIPE_BUF        PIPE_BUFSIZ

#define OPEN_MAX        NR_OPEN

#ifdef __WATCOMC__
#include <watcom/limits.h>
#endif

#if defined(__GNUC__) && !defined(_GCC_NEXT_LIMITS_H)
#include_next <limits.h>
#endif

#endif
