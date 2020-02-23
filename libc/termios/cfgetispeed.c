#include <termios.h>

speed_t
cfgetispeed(struct termios *tp)
{
	return tp->c_cflag & CBAUD;
}
