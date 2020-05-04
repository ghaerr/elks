#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

pid_t
wait(int * status)
{
	int ret;

	/* for compatibility, loop while EINTR return*/
	do {
		ret = wait4(-1, status, 0, (void*)0);
	} while (ret == -1 && errno == EINTR);

	return ret;
}
