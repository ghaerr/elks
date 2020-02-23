#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>

int
tcsetattr(int fd, int optional_actions, const struct termios *termios_p)
{
	switch (optional_actions)
	{
		case TCSANOW:
			return ioctl(fd, TCSETS, termios_p);

		case TCSADRAIN:
			return ioctl(fd, TCSETSW, termios_p);

		case TCSAFLUSH:
			return ioctl(fd, TCSETSF, termios_p);

		default:
			errno = EINVAL;
			return -1;
	}
}
