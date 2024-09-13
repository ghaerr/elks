/****************************************************************************
*
* Description:  ELKS mount() system call.
*
****************************************************************************/

#include <sys/mount.h>
#include "watcom/syselks.h"

int mount(const char *__dev, const char *__dir, int __type, int __flags)
{
    sys_setseg(__dev);
    syscall_res res = sys_call4( SYS_mount, (unsigned)__dev, (unsigned)__dir, __type,
        __flags);
    __syscall_return( int, res );
}
