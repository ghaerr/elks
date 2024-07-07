/****************************************************************************
*
* Description:  ELKS gettimeofday() system call.
*
****************************************************************************/

#include "watcom/syselks.h"

int gettimeofday(struct timeval * restrict __tv, void * restrict __tzp)
{
    sys_setseg(__tv);
    syscall_res res = sys_call2(SYS_gettimeofday, (unsigned)__tv, (unsigned)__tzp);
    __syscall_return(int, res);
}
