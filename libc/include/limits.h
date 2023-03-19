#ifndef __LIMITS_H
#define __LIMITS_H

/* Maximum number of bytes in a pathname, including the terminating null. */
#define PATH_MAX 128

/* Maximum number of bytes in a filename, not including terminating null. */
/* copied from MAXNAMLEN in linuxmt/dirent.h */
#define NAME_MAX 26

/* copied from PIPE_BUFSIZ in linuxmt/config.h */
#define PIPE_BUF 80

/* copied from NR_OPEN in linuxmt/fs.h */
#define OPEN_MAX 20

#include_next <limits.h>

#endif
