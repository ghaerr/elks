#ifndef __LIMITS_H
#define __LIMITS_H

/* Maximum number of bytes in a pathname, including the terminating null. */
#define PATH_MAX 128

/* Maximum number of bytes in a filename, not including terminating null. */
#define NAME_MAX 14

#define PIPE_BUF 80

#define OPEN_MAX 20

#include_next <limits.h>

#endif
