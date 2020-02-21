#include <termios.h>

int
cfsetispeed(struct termios *tp, speed_t speed)
{
	return cfsetospeed(tp, speed);
}
