/****************************************************************************
*
* Description:  ELKS connect() system call.
*
****************************************************************************/

#include <sys/socket.h>
#include "watcom/syselks.h"


int connect(int __sock, const struct sockaddr *__address, socklen_t __address_len)
{
    sys_setseg(__address);
    syscall_res res = sys_call3(SYS_connect, __sock, (unsigned)__address, __address_len);
    __syscall_return(int, res);
}
