/****************************************************************************
*
* Description:  ELKS listen() system call.
*
****************************************************************************/

#include <sys/socket.h>
#include "watcom/syselks.h"


int listen(int __sock, int __backlog)
{
    syscall_res res = sys_call2(SYS_listen, __sock, __backlog);
    __syscall_return(int, res);
}
