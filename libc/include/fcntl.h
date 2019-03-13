#ifndef __FCNTL__H
#define __FCNTL__H

#include <features.h>
#include <sys/types.h>
#include __SYSINC__(fcntl.h)

#ifndef FNDELAY
#define FNDELAY	O_NDELAY
#endif

__BEGIN_DECLS

extern int creat __P ((__const char * __filename, mode_t __mode));
extern int fcntl __P ((int __fildes,int __cmd, ...));
extern int open __P ((__const char * __filename, int __flags, ...));

__END_DECLS

#endif
