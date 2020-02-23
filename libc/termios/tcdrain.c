#ifdef L_tcdrain
#include <termios.h>

/* Wait for pending output to be written on FD.  */
int
tcdrain(int fd)
{
	/* With an argument of 1, TCSBRK just waits for output to drain.  */
	return ioctl(fd, TCSBRK, 1);
}
#endif
