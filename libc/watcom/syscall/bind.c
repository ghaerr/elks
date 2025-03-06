/****************************************************************************
*
* Description:  ELKS bind() system call.
*
****************************************************************************/

#include <sys/socket.h>
#include "watcom/syselks.h"


int bind(int __sock, const struct sockaddr *__address, socklen_t __address_len)
{
    sys_setseg(__address);
    syscall_res res = sys_call3(SYS_bind, __sock, (unsigned)__address, __address_len);
    __syscall_return(int, res);
}
