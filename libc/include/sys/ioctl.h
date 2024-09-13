#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H

#include <features.h>
#include __SYSINC__(ioctl.h)

int ioctl(int __fildes, int __cmd, ...);         /* syscall */

#endif
