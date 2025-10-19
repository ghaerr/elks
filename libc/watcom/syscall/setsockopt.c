/****************************************************************************
*
* Description:  ELKS setsockopt() system call.
*
****************************************************************************/

#include <sys/socket.h>
#include "watcom/syselks.h"


int setsockopt(int __sock, int __level, int __option_name, const void *__option_value,
    socklen_t __option_len)
{
    sys_setseg(__option_value);
    syscall_res res = sys_call5(SYS_setsockopt, __sock, __level, __option_name,
        (unsigned)__option_value, __option_len);
    __syscall_return(int, res);
}
