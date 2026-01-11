#ifdef L_tcsendbreak
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>

/* Send zero bits on FD.  */
int
tcsendbreak(int fd, int duration)
{
    /* The break lasts 0.25 to 0.5 seconds and duration is ignored */
    return ioctl(fd, TCSBRK, 0);
}
#endif
