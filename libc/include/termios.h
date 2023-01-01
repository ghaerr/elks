#ifndef __TERMIOS_H
#define __TERMIOS_H

#include <features.h>
#include <sys/types.h>
#include __SYSINC__(termios.h)

speed_t cfgetispeed(struct termios *__termios_p);
speed_t cfgetospeed(struct termios *__termios_p);
int cfsetispeed(struct termios *__termios_p, speed_t __speed);
int cfsetospeed(struct termios *__termios_p, speed_t __speed);

void cfmakeraw (struct termios *__t);

int tcgetattr (int fd, struct termios * termios_p);
int tcsetattr (int fd, int optional_actions, const struct termios * termios_p);

int tcdrain(int __fildes);
int tcflow(int __fildes, int __action);
int tcflush(int __fildes, int __queue_selector);
int tcsendbreak(int __fildes, int __duration);
pid_t tcgetpgrp(int __fildes);
int tcsetpgrp(int __fildes, pid_t __pgrp_id);

#endif
