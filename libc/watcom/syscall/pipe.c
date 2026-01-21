/****************************************************************************
*
* Description:  ELKS pipedes() system call.
*
****************************************************************************/

#include <sys/socket.h>
#include "watcom/syselks.h"


int
pipe(int __pipedes[2])
{
	syscall_res res;

    sys_setseg(__pipedes);
	res = sys_call1(SYS_pipe, (unsigned)__pipedes);
	__syscall_return(int, res);
}
