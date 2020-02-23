#include <sys/ioctl.h>
#include <termios.h>

/* Flush pending data on FD.  */
int
tcflush(int fd, int queue_selector)
{
	return ioctl(fd, TCFLSH, queue_selector);
}
