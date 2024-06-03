#ifdef L_tcsendbreak
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>

/* Send zero bits on FD.  */
int
tcsendbreak(int fd, int duration)
{
	/*
	 * The break lasts 0.25 to 0.5 seconds if DURATION is zero, and an
	 * implementation-defined period if DURATION is nonzero. We define a
	 * positive DURATION to be number of milliseconds to break.
	 */
	if (duration <= 0)
		return ioctl(fd, TCSBRK, 0);

	/*
	 * ioctl can't send a break of any other duration for us. This could be
	 * changed to use trickery (e.g. lower speed and send a '\0') to send
	 * the break, but for now just return an error.
	 */
	errno = EINVAL;
	return -1;
}
#endif
