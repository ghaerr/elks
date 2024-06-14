/****************************************************************************
*
* Description:  ELKS _fmemalloc() system call.
*
****************************************************************************/

#include <malloc.h>
#include "watcom/syselks.h"

int _fmemalloc( int __paras, seg_t *__pseg )
{
    sys_setseg(__pseg);
    syscall_res res = sys_call2( SYS_fmemalloc, __paras, (unsigned)__pseg);
    __syscall_return( int, res );
}
