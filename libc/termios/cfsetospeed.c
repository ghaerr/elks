#include <termios.h>

int
cfsetospeed(struct termios *tp, speed_t speed)
{
#ifdef CBAUDEX
	if ((speed & ~CBAUD)
	|| ((speed & CBAUDEX) && (speed < B57600 || speed > B115200)))
		return 0;
#else
	if (speed & ~CBAUD)
		return 0;
#endif
	tp->c_cflag &= ~CBAUD;
	tp->c_cflag |= speed;

	return 0;
}
