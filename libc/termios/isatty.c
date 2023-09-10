#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>

int
isatty(int fd)
{
	struct termios term;
	int rv, err = errno;

	rv = (ioctl(fd, TCGETS, &term) == 0);

	if(rv == 0 && errno == ENOSYS)
		rv = (fd < 3);

	errno = err;

	return rv;
}
