#ifndef __FCNTL__H
#define __FCNTL__H

#include <features.h>
#include <sys/types.h>
#include __SYSINC__(fcntl.h)

#ifndef FNDELAY
#define FNDELAY	O_NDELAY
#endif

int creat(const char * __filename, mode_t __mode);
int fcntl(int __fildes,int __cmd, ...);
int open(const char * __filename, int __flags, ...);         /* syscall */

#endif
