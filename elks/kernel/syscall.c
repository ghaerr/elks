#include <linuxmt/errno.h>

int syscall(void)
{
    return -ENOSYS;
}
