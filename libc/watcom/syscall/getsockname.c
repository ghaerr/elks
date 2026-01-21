/****************************************************************************
*
* Description:  ELKS getsocknam() system call.
*
****************************************************************************/

#include <sys/socket.h>
#include "watcom/syselks.h"


int
getsockname(int __sock, struct sockaddr *__address, socklen_t *__address_len)
{
	syscall_res res;

    sys_setseg(__address);
	res = sys_call4(SYS_getsocknam, __sock, (unsigned)__address, (unsigned)__address_len,
        0);
	__syscall_return(int, res);
}
