#ifdef L_tcflow
#include <termios.h>

int
tcflow(int fd, int action)
{
	return ioctl(fd, TCXONC, action);
}
#endif
