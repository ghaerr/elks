/****************************************************************************
*
* Description:  ELKS umount() system call.
*
****************************************************************************/

#include <sys/mount.h>
#include "watcom/syselks.h"

int umount(const char *__dir)
{
    sys_setseg(__dir);
    syscall_res res = sys_call1( SYS_umount, (unsigned)__dir);
    __syscall_return( int, res );
}
