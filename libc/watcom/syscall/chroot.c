/****************************************************************************
*
* Description:  ELKS chroot() system call.
*
****************************************************************************/

#include <sys/socket.h>
#include "watcom/syselks.h"


int
chroot(const char *__dirname)
{
	syscall_res res;

    sys_setseg(__dirname);
	res = sys_call1(SYS_chroot, (unsigned)__dirname);
	__syscall_return(int, res);
}
