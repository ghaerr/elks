/****************************************************************************
*
* Description:  ELKS socket() system call.
*
****************************************************************************/

#include <sys/socket.h>
#include "watcom/syselks.h"


int socket(int __domain, int __type, int __protocol)
{
    syscall_res res = sys_call3(SYS_socket, __domain, __type, __protocol);
    __syscall_return(int, res);
}
