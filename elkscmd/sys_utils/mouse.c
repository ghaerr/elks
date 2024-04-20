/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * This program opens a serial port directly, and interprets serial data.
 * Microsoft, PC, and Logitech mice are supported.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>

#define	MOUSE_DEVICE "/dev/ttyS0"	/* mouse tty device*/
#define	MOUSE_MICROSOFT		1		/* microsoft mouse*/
#define	MOUSE_PC			0		/* pc/logitech mouse*/
#define MAX_BYTES	128				/* number of bytes for buffer*/

/* states for the mouse*/
#define	IDLE			0		/* start of byte sequence */
#define	XSET			1		/* setting x delta */
#define	YSET			2		/* setting y delta */
#define	XADD			3		/* adjusting x delta */
#define	YADD			4		/* adjusting y delta */

/* pc and ms button codes*/
#define	PC_LEFT_BUTTON		4
#define PC_MIDDLE_BUTTON	2
#define PC_RIGHT_BUTTON		1
#define	MS_LEFT_BUTTON		2
#define MS_RIGHT_BUTTON		1

/* final button codes*/
#define LBUTTON				0x01
#define MBUTTON				0x10
#define RBUTTON				0x02

/* Bit fields in the bytes sent by the mouse.*/
#define TOP_FIVE_BITS		0xf8
#define BOTTOM_THREE_BITS	0x07
#define TOP_BIT				0x80
#define SIXTH_BIT			0x40
#define BOTTOM_TWO_BITS		0x03
#define THIRD_FOURTH_BITS	0x0c
#define BOTTOM_SIX_BITS  	0x3f

/* local data*/
static int		rawmode;    /* show raw mouse data */
static int		mouse_fd;	/* file descriptor for mouse */
static int		state;		/* IDLE, XSET, ... */
static int		buttons;	/* current mouse buttons pressed*/
static int		availbuttons;	/* which buttons are available */
static int		xd;			/* change in x */
static int		yd;			/* change in y */

static int		left;		/* because the button values change */
static int		middle;		/* between mice, the buttons are */
static int		right;		/* redefined */

static unsigned char	*bp;/* buffer pointer */
static int		nbytes;		/* number of bytes left */
static unsigned char	buffer[MAX_BYTES];	/* data bytes read */
static int		(*parse)();	/* parse routine */

/* local routines*/
static int  	read_mouse(int *dx, int *dy, int *dz, int *bptr);
int  	        parsePC(int);		/* routine to interpret PC mouse */
int  	        parseMS(int);		/* routine to interpret MS mouse */

/*
 * Open up the mouse device.
 * Returns the fd if successful, or negative if unsuccessful.
 */
int
open_mouse(void)
{
	struct termios termios;

	/* set button bits and parse procedure*/
#if MOUSE_PC
	/* pc or logitech mouse*/
	left = PC_LEFT_BUTTON;
	middle = PC_MIDDLE_BUTTON;
	right = PC_RIGHT_BUTTON;
	parse = parsePC;
#elif MOUSE_MICROSOFT
	/* microsoft mouse*/
	left = MS_LEFT_BUTTON;
	right = MS_RIGHT_BUTTON;
	middle = 0;
	parse = parseMS;
#else
	return -1;
#endif

	/* open mouse port*/
	mouse_fd = open(MOUSE_DEVICE, O_EXCL | O_NOCTTY | O_NONBLOCK);
	if (mouse_fd < 0) {
		printf("Can't open %s, error %d\n", MOUSE_DEVICE, errno);
 		return -1;
	}

	/* set rawmode serial port using termios*/
	if (tcgetattr(mouse_fd, &termios) < 0) {
		printf("Can't get termio on %s, error %d\n", MOUSE_DEVICE, errno);
		close(mouse_fd);
		return -1;
	}

	if(cfgetispeed(&termios) != B1200)
		cfsetispeed(&termios, B1200);

	termios.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
	termios.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON | BRKINT | IGNBRK);
	termios.c_cflag &= ~(CSIZE | PARENB);
	termios.c_cflag |= CS8;
	termios.c_cc[VMIN] = 0;
	termios.c_cc[VTIME] = 0;
	if(tcsetattr(mouse_fd, TCSAFLUSH, &termios) < 0) {
		printf("Can't set termio on %s, error %d\n", MOUSE_DEVICE, errno);
		close(mouse_fd);
		return -1;
	}

	/* initialize data*/
	availbuttons = left | middle | right;
	state = IDLE;
	nbytes = 0;
	buttons = 0;
	xd = 0;
	yd = 0;

	return mouse_fd;
}

/*
 * Attempt to read bytes from the mouse and interpret them.
 * Returns -1 on error, 0 if either no bytes were read or not enough
 * was read for a complete state, or 1 if the new state was read.
 * When a new state is read, the current buttons and x and y deltas
 * are returned.  This routine does not block.
 */
int
read_mouse(int *dx, int *dy, int *dz, int *bptr)
{
	int	b;

	/*
	 * If there are no more bytes left, then read some more,
	 * waiting for them to arrive.  On a signal or a non-blocking
	 * error, return saying there is no new state available yet.
	 */
	if (nbytes <= 0) {
		bp = buffer;
		nbytes = read(mouse_fd, bp, MAX_BYTES);
		if (nbytes < 0) {
			if (errno == EINTR || errno == EAGAIN)
				return 0;
			return -1;
		}
	}
	if (rawmode) {
		while (nbytes-- > 0)
			printf("%02x ", *bp++);
		return 0;
	}

	/*
	 * Loop over all the bytes read in the buffer, parsing them.
	 * When a complete state has been read, return the results,
	 * leaving further bytes in the buffer for later calls.
	 */
	while (nbytes-- > 0) {
		if ((*parse)((int) *bp++)) {
			*dx = xd;
			*dy = yd;
			*dz = 0;
			b = 0;
			if(buttons & left)
				b |= LBUTTON;
			if(buttons & right)
				b |= RBUTTON;
			if(buttons & middle)
				b |= MBUTTON;
			*bptr = b;
			return 1;
		}
	}

	return 0;
}

#if MOUSE_PC
/*
 * Input routine for PC mouse.
 * Returns nonzero when a new mouse state has been completed.
 */
int
parsePC(int byte)
{
	int	sign;			/* sign of movement */

	switch (state) {
		case IDLE:
			if ((byte & TOP_FIVE_BITS) == TOP_BIT) {
				buttons = ~byte & BOTTOM_THREE_BITS;
				state = XSET;
			}
			break;

		case XSET:
			sign = 1;
			if (byte > 127) {
				byte = 256 - byte;
				sign = -1;
			}
			xd = byte * sign;
			state = YSET;
			break;

		case YSET:
			sign = 1;
			if (byte > 127) {
				byte = 256 - byte;
				sign = -1;
			}
			yd = -byte * sign;
			state = XADD;
			break;

		case XADD:
			sign = 1;
			if (byte > 127) {
				byte = 256 - byte;
				sign = -1;
			}
			xd += byte * sign;
			state = YADD;
			break;

		case YADD:
			sign = 1;
			if (byte > 127) {
				byte = 256 - byte;
				sign = -1;
			}
			yd -= byte * sign;
			state = IDLE;
			return 1;
	}
	return 0;
}
#endif

#if MOUSE_MICROSOFT
/*
 * Input routine for Microsoft mouse.
 * Returns nonzero when a new mouse state has been completed.
 */
int
parseMS(int byte)
{
	switch (state) {
		case IDLE:
			if (byte & SIXTH_BIT) {
				buttons = (byte >> 4) & BOTTOM_TWO_BITS;
				yd = ((byte & THIRD_FOURTH_BITS) << 4);
				xd = ((byte & BOTTOM_TWO_BITS) << 6);
				state = XADD;
			}
			break;

		case XADD:
			xd |= (byte & BOTTOM_SIX_BITS);
			state = YADD;
			break;

		case YADD:
			yd |= (byte & BOTTOM_SIX_BITS);
			state = IDLE;
			if (xd > 127)
				xd -= 256;
			if (yd > 127)
				yd -= 256;
			return 1;
	}
	return 0;
}
#endif

int main(int argc, char **argv)
{
	int x, y, z, b;

	rawmode = (argc > 1);

	if(open_mouse() < 0)
		return 1;

	printf("[Mouse test, ^C to quit]\n");
	while(1) {
		if(read_mouse(&x, &y, &z, &b) == 1) {
			printf("x:%4d, y:%4d, b:%2d\n", x, y, b);
		}
	}
	return 0;
}
