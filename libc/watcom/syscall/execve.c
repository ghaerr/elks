/****************************************************************************
*
* Description:  ELKS _execve() system call.
*
****************************************************************************/

#include <unistd.h>
#include "watcom/syselks.h"

int _execve(const char *__fname, char *__stk_ptr, int __stack_bytes)
{
    sys_setseg(__fname);
    syscall_res res = sys_call3( SYS_execve, (unsigned)__fname, (unsigned)__stk_ptr,
        __stack_bytes);
    __syscall_return( int, res );
}
