#include <unistd.h>

pid_t
getpid(void)
{
	int ppid;
	return _getpid (&ppid);
}
