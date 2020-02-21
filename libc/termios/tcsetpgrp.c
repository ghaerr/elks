#ifdef L_tcsetpgrp
#include <termios.h>
/* Set the foreground process group ID of FD set PGRP_ID.  */
int
tcsetpgrp(int fd, pid_t pgrp_id)
{
	return ioctl(fd, TIOCSPGRP, &pgrp_id);
}
#endif
