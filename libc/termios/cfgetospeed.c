#include <termios.h>

speed_t
cfgetospeed(struct termios *tp)
{
	return tp->c_cflag & CBAUD;
}
