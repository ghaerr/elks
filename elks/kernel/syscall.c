#include <linuxmt/errno.h>

int syscall()
{
	return -ENOSYS;
}
