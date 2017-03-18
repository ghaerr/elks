/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * /dev/tty TTY Keyboard Driver
 */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include "../device.h"

#if ELKS
#define	KEYBOARD	"/dev/tty1"	/* keyboard associated with screen*/
#else
#define	KEYBOARD	"/dev/tty"	/* keyboard associated with screen*/
#endif

static int  TTY_Open(KBDDEVICE *pkd);
static void TTY_Close(void);
static void TTY_GetModifierInfo(MODIFIER *modifiers);
static int  TTY_Read(UCHAR *buf, MODIFIER *modifiers);

KBDDEVICE kbddev = {
	TTY_Open,
	TTY_Close,
	TTY_GetModifierInfo,
	TTY_Read,
	NULL
};

static	int		fd;		/* file descriptor for keyboard */
static	struct termios	old;		/* original terminal modes */

/*
 * Open the keyboard.
 * This is real simple, we just use a special file handle
 * that allows non-blocking I/O, and put the terminal into
 * character mode.
 */
static int
TTY_Open(KBDDEVICE *pkd)
{
	struct termios	new;	/* new terminal modes */

	fd = open(KEYBOARD, O_NONBLOCK);
	if (fd < 0)
		return -1;

	if (tcgetattr(fd, &old) < 0) {
		close(fd);
		fd = 0;
		return -1;
	}

	new = old;
	/* If you uncomment ISIG and BRKINT below, then ^C will be ignored.*/
	new.c_lflag &= ~(ECHO | ICANON | IEXTEN /*| ISIG*/);
	new.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON /*| BRKINT*/);
	new.c_cflag &= ~(CSIZE | PARENB);
	new.c_cflag |= CS8;
	new.c_cc[VMIN] = 0;
	new.c_cc[VTIME] = 0;
	if(tcsetattr(fd, TCSAFLUSH, &new) < 0) {
		close(fd);
		fd = 0;
		return -1;
	}
	return fd;

}

/*
 * Close the keyboard.
 * This resets the terminal modes.
 */
static void
TTY_Close(void)
{
	tcsetattr(fd, TCSANOW, &old);
	close(fd);
	fd = 0;
}

/*
 * Return the possible modifiers for the keyboard.
 */
static  void
TTY_GetModifierInfo(MODIFIER *modifiers)
{
	*modifiers = 0;			/* no modifiers available */
}

/*
 * This reads one keystroke from the keyboard, and the current state of
 * the mode keys (ALT, SHIFT, CTRL).  Returns -1 on error, 0 if no data
 * is ready, and 1 if data was read.  This is a non-blocking call.
 */
static int
TTY_Read(UCHAR *buf, MODIFIER *modifiers)
{
	int	cc;			/* characters read */

	*modifiers = 0;			/* no modifiers yet */
	cc = read(fd, buf, 1);
	if (cc > 0) {
		if(*buf == 0x1b)
			return -2;	/* special case ESC*/
		return 1;
	}
	if ((cc < 0) && (errno != EINTR) && (errno != EAGAIN))
		return -1;
	return 0;
}
