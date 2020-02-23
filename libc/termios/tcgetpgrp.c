#ifdef L_tcgetpgrp
#include <termios.h>

/* Return the foreground process group ID of FD.  */
pid_t
tcgetpgrp(int fd)
{
	int pgrp;

	if (ioctl(fd, TIOCGPGRP, &pgrp) < 0)
		return (pid_t) - 1;

	return (pid_t) pgrp;
}
#endif
