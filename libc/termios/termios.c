/* Copyright (C) 1996 Robert de Bath <robert@mayday.compulink.co.uk> This
 * file is part of the Linux-8086 C library and is distributed under the
 * GNU Library General Public License.
 */

/* Note: This is based loosely on the Glib termios routines. */

#include <errno.h>
#include <stddef.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

//-----------------------------------------------------------------------------

int isatty (int fd)
{
   struct termios term;
   int rv, err = errno;
   rv= (ioctl(fd, TCGETS, &term)==0);
   if( rv==0 && errno == ENOSYS )
      rv = (fd<3);
   errno = err;
   return rv;
}

//-----------------------------------------------------------------------------

int tcgetattr (int fd, struct termios * termios_p)
{
   return ioctl(fd, TCGETS, termios_p);
}

//-----------------------------------------------------------------------------

int tcsetattr (int fd, int optional_actions, const struct termios * termios_p)
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

//-----------------------------------------------------------------------------

#ifdef L_tcdrain
/* Wait for pending output to be written on FD.  */
int
tcdrain(fd)
int   fd;
{
   /* With an argument of 1, TCSBRK just waits for output to drain.  */
   return ioctl(fd, TCSBRK, 1);
}
#endif

#ifdef L_tcflow
int 
tcflow(fd, action)
int fd;
int action;
{
   return ioctl(fd, TCXONC, action);
}
#endif

/* Flush pending data on FD.  */
int
tcflush(fd, queue_selector)
int   fd;
int   queue_selector;
{
   return ioctl(fd, TCFLSH, queue_selector);
}

/* Send zero bits on FD.  */
int
tcsendbreak(fd, duration)
int   fd;
int   duration;
{
   /*
    * The break lasts 0.25 to 0.5 seconds if DURATION is zero, and an
    * implementation-defined period if DURATION is nonzero. We define a
    * positive DURATION to be number of milliseconds to break.
    */
   if (duration <= 0)
      return ioctl(fd, TCSBRK, 0);

   /*
    * ioctl can't send a break of any other duration for us. This could be
    * changed to use trickery (e.g. lower speed and send a '\0') to send
    * the break, but for now just return an error.
    */
   errno = EINVAL;
   return -1;
}

#ifdef L_tcsetpgrp
/* Set the foreground process group ID of FD set PGRP_ID.  */
int
tcsetpgrp(fd, pgrp_id)
int   fd;
pid_t pgrp_id;
{
   return ioctl(fd, TIOCSPGRP, &pgrp_id);
}
#endif

#ifdef L_tcgetpgrp
/* Return the foreground process group ID of FD.  */
pid_t
tcgetpgrp(fd)
int   fd;
{
   int   pgrp;
   if (ioctl(fd, TIOCGPGRP, &pgrp) < 0)
      return (pid_t) - 1;
   return (pid_t) pgrp;
}
#endif

speed_t cfgetospeed(tp)
struct termios *tp;
{
    return (tp->c_cflag & CBAUD);
}

speed_t cfgetispeed(tp)
struct termios *tp;
{
    return (tp->c_cflag & CBAUD);
}

int cfsetospeed(tp, speed)
struct termios *tp; speed_t speed;
{
#ifdef CBAUDEX
    if ((speed & ~CBAUD) || 
	((speed & CBAUDEX) && (speed < B57600 || speed > B115200)))
	return 0;
#else
    if (speed & ~CBAUD)
	return 0;
#endif
    tp->c_cflag &= ~CBAUD;
    tp->c_cflag |= speed;

    return 0;
}

int cfsetispeed(tp, speed)
struct termios *tp; speed_t speed;
{
    return cfsetospeed(tp, speed);
}
 
/* From linux libc-4.6.27 again */
#ifdef L_cfmakeraw
/* Copyright (C) 1992 Free Software Foundation, Inc.
This file is part of the GNU C Library.*/

void
cfmakeraw(t)
struct termios *t;
{
/* I changed it to the current form according to the suggestions 
 * from Bruce Evans. Thanks Bruce. Please report the problems to
 * H.J. Lu (hlu@eecs.wsu.edu).
 */

/*
 * I took out the bits commented out by #if 1...#else    - RHP
 */

    /*  VMIN = 0 means non-blocking for Linux */
    t->c_cc[VMIN] = 1; t->c_cc[VTIME] = 1;
    /* clear some bits with &= ~(bits), set others with |= */
    t->c_cflag &= ~(CSIZE|PARENB|CSTOPB);
    t->c_cflag |=  (CS8|HUPCL|CREAD);
    t->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|INPCK|ISTRIP);
    t->c_iflag &= ~(INLCR|IGNCR|ICRNL|IXON|IXOFF);
    t->c_iflag |=  (BRKINT|IGNPAR);
    t->c_oflag &= ~(OPOST|OLCUC|OCRNL|ONOCR|ONLRET|OFILL|OFDEL);
    t->c_oflag &= ~(NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY);
    t->c_oflag |=  (ONLCR|NL0|CR0|TAB3|BS0|VT0|FF0);
    t->c_lflag &= ~(ISIG|ICANON|IEXTEN|ECHO|ECHOE|ECHOK|ECHONL);
    t->c_lflag &= ~(NOFLSH|XCASE);
    t->c_lflag &= ~(ECHOPRT|ECHOCTL|ECHOKE);
}
#endif
