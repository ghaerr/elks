/****************************************************************************
*
* Description:  ELKS accept() system call.
*
****************************************************************************/

#include <sys/socket.h>
#include "watcom/syselks.h"


int accept(int __sock, struct sockaddr * restrict __address,
    socklen_t * restrict __address_len)
{
    sys_setseg(__address);
    sys_setseg(__address_len);
    syscall_res res = sys_call3(SYS_accept, __sock, (unsigned)__address,
        (unsigned)__address_len);
    __syscall_return(int, res);
}
