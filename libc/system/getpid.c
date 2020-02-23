#include <unistd.h>

extern pid_t _getpid (pid_t *);

pid_t getpid(void)
{
	pid_t ppid;
	return _getpid (&ppid);
}
