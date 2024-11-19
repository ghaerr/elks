/****************************************************************************
*
* Description:  ELKS _fmemfree() system call.
*
****************************************************************************/

#include <malloc.h>
#include "watcom/syselks.h"

int _fmemfree( unsigned short __seg )
{
    syscall_res res = sys_call1( SYS_fmemfree, __seg);
    __syscall_return( int, res );
}
