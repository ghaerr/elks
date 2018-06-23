#ifndef __TERMIOS_H
#define __TERMIOS_H

#include <features.h>
#include <sys/types.h>
#include __SYSINC__(termios.h)

extern speed_t cfgetispeed __P ((struct termios *__termios_p));
extern speed_t cfgetospeed __P ((struct termios *__termios_p));
extern int cfsetispeed __P ((struct termios *__termios_p, speed_t __speed));
extern int cfsetospeed __P ((struct termios *__termios_p, speed_t __speed));

extern void cfmakeraw  __P ((struct termios *__t));

int tcgetattr (int fd, struct termios * termios_p);
int tcsetattr (int fd, int optional_actions, const struct termios * termios_p);

extern int tcdrain __P ((int __fildes));
extern int tcflow __P ((int __fildes, int __action));
extern int tcflush __P ((int __fildes, int __queue_selector));
extern int tcsendbreak __P ((int __fildes, int __duration));
extern pid_t tcgetpgrp __P ((int __fildes));
extern int tcsetpgrp __P ((int __fildes, pid_t __pgrp_id));

#endif
