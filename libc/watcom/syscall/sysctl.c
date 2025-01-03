/****************************************************************************
*
* Description:  ELKS sysctl() system call.
*
****************************************************************************/

#include <malloc.h>
#include "watcom/syselks.h"

int sysctl( int op, char * __name, int * __value)
{
    sys_setseg(__name);
    sys_setseg(__value);
    syscall_res res = sys_call3( SYS_sysctl, op, (unsigned)__name, (unsigned)__value);
    __syscall_return( int, res );
}
