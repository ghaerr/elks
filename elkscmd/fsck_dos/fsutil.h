#ifndef _FS_UTIL_H
#define _FS_UTIL_H

#define pwarn printf
#define pfatal printf

typedef unsigned int    u_int;      /* FIXME possibly change to 32-bit for ELKS */
typedef unsigned char   u_char;
typedef unsigned int    uint;
typedef unsigned long   u_int32_t;
typedef unsigned long   U32;

#include <limits.h>
#define MAXPATHLEN  PATH_MAX
typedef off_t   loff_t;
#define lseek64 lseek

#endif
