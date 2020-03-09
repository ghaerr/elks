/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * UNIX Serial Port Mouse Driver
 *
 * This driver opens a serial port directly, and interprets serial data.
 * Microsoft, PC, and Logitech mice are supported.
 *
 * The following environment variables control the mouse type expected
 * and the serial port to open.
 *
 * Environment Var	Default		Allowed
 * MOUSE_TYPE		pc		ms, pc, logi
 * MOUSE_PORT		/dev/ttyS1	any serial port
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "../device.h"

#define TERMIOS		1	/* set to use termios serial port control*/
#define SGTTY		0	/* set to use sgtty serial port control*/

#define	SCALE		3	/* default scaling factor for acceleration */
#define	THRESH		5	/* default threshhold for acceleration */

#if TERMIOS
#include <termios.h>
#endif
#if SGTTY
#include <sgtty.h>
#endif

/* default settings*/
#define	MOUSE_PORT	"/dev/ttyS0"	/* default mouse tty port */
#define	MOUSE_TYPE	"ms"			/* default mouse type ("ms" or "pc") */
#define MAX_BYTES	128				/* number of bytes for buffer */

/* states for the mouse*/
#define	IDLE			0		/* start of byte sequence */
#define	XSET			1		/* setting x delta */
#define	YSET			2		/* setting y delta */
#define	XADD			3		/* adjusting x delta */
#define	YADD			4		/* adjusting y delta */

/* values in the bytes returned by the mouse for the buttons*/
#define	PC_LEFT_BUTTON		4
#define PC_MIDDLE_BUTTON	2
#define PC_RIGHT_BUTTON		1

#define	MS_LEFT_BUTTON		2
#define MS_RIGHT_BUTTON		1

/* Bit fields in the bytes sent by the mouse.*/
#define TOP_FIVE_BITS		0xf8
#define BOTTOM_THREE_BITS	0x07
#define TOP_BIT			0x80
#define SIXTH_BIT		0x40
#define BOTTOM_TWO_BITS		0x03
#define THIRD_FOURTH_BITS	0x0c
#define BOTTOM_SIX_BITS  	0x3f

/* local data*/
static int		mouse_fd;	/* file descriptor for mouse */
static int		state;		/* IDLE, XSET, ... */
static BUTTON		buttons;	/* current mouse buttons pressed*/
static BUTTON		availbuttons;	/* which buttons are available */
static COORD		xd;		/* change in x */
static COORD		yd;		/* change in y */

static int		left;		/* because the button values change */
static int		middle;		/* between mice, the buttons are */
static int		right;		/* redefined */

static unsigned char	*bp;		/* buffer pointer */
static int		nbytes;		/* number of bytes left */
static unsigned char	buffer[MAX_BYTES];	/* data bytes read */
static int		(*parse)();	/* parse routine */

/* local routines*/
static int  	MOU_Open(MOUSEDEVICE *pmd);
static void 	MOU_Close(void);
static BUTTON  	MOU_GetButtonInfo(void);
static void	MOU_GetDefaultAccel(int *pscale,int *pthresh);
static int  	MOU_Read(COORD *dx, COORD *dy, COORD *dz, BUTTON *bptr);
static int  	ParsePC(int);		/* routine to interpret PC mouse */
static int  	ParseMS(int);		/* routine to interpret MS mouse */

MOUSEDEVICE mousedev = {
	MOU_Open,
	MOU_Close,
	MOU_GetButtonInfo,
	MOU_GetDefaultAccel,
	MOU_Read,
	NULL
};

/*
 * Open up the mouse device.
 * Returns the fd if successful, or negative if unsuccessful.
 */
static int
MOU_Open(MOUSEDEVICE *pmd)
{
	char	*type;
	char	*port;
	struct termios termios;

	/* get mouse type and port*/
	if( !(type = getenv("MOUSE_TYPE")))
		type = MOUSE_TYPE;

	if( !(port = getenv("MOUSE_PORT")))
		port = MOUSE_PORT;

	/* set button bits and parse procedure*/
	if(!strcmp(type, "pc") || !strcmp(type, "logi")) {
		/* pc or logitech mouse*/
		left = PC_LEFT_BUTTON;
		middle = PC_MIDDLE_BUTTON;
		right = PC_RIGHT_BUTTON;
		parse = ParsePC;
	} else if (strcmp(type, "ms") == 0) {
		/* microsoft mouse*/
		left = MS_LEFT_BUTTON;
		right = MS_RIGHT_BUTTON;
		middle = 0;
		parse = ParseMS;
	} else
		return -1;

	/* open mouse port*/
	mouse_fd = open(port, O_NONBLOCK);
	if (mouse_fd < 0) {
		fprintf(stderr,
			"Error %d opening serial mouse type %s on port %s.\n",
			errno, type, port);
 		return -1;
	}

#if SGTTY
	/* set rawmode serial port using sgtty*/
	struct sgttyb sgttyb;

	if (ioctl(fd, TIOCGETP, &sgttyb) == -1)
		goto err;
	sgttyb.sg_flags |= RAW;
	sgttyb.sg_flags &= ~(EVENP | ODDP | ECHO | XTABS | CRMOD);

	if (ioctl(fd, TIOCSETP, &sgttyb) == -1)
		goto err;
	if (ioctl(fd, TIOCFLUSH, 0) < 0)
		goto err;
#endif

#if TERMIOS
	/* set rawmode serial port using termios*/
	if (tcgetattr(mouse_fd, &termios) < 0)
		goto err;

	if(cfgetispeed(&termios) != B1200)
		cfsetispeed(&termios, B1200);

	termios.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
	termios.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON | BRKINT | IGNBRK);
	termios.c_cflag &= ~(CSIZE | PARENB);
	termios.c_cflag |= CS8;
	termios.c_cc[VMIN] = 0;
	termios.c_cc[VTIME] = 0;
	if(tcsetattr(mouse_fd, TCSAFLUSH, &termios) < 0)
		goto err;
#endif

	/* initialize data*/
	availbuttons = left | middle | right;
	state = IDLE;
	nbytes = 0;
	buttons = 0;
	xd = 0;
	yd = 0;

	return mouse_fd;
err:
	close(mouse_fd);
	mouse_fd = 0;
	return -1;
}

/*
 * Close the mouse device.
 */
static void
MOU_Close(void)
{
	if (mouse_fd > 0)
		close(mouse_fd);
	mouse_fd = 0;
}

/*
 * Get mouse buttons supported
 */
static BUTTON
MOU_GetButtonInfo(void)
{
	return availbuttons;
}

/*
 * Get default mouse acceleration settings
 */
static void
MOU_GetDefaultAccel(int *pscale,int *pthresh)
{
	*pscale = SCALE;
	*pthresh = THRESH;
}

/*
 * Attempt to read bytes from the mouse and interpret them.
 * Returns -1 on error, 0 if either no bytes were read or not enough
 * was read for a complete state, or 1 if the new state was read.
 * When a new state is read, the current buttons and x and y deltas
 * are returned.  This routine does not block.
 */
static int
MOU_Read(COORD *dx, COORD *dy, COORD *dz, BUTTON *bptr)
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

/*
 * Input routine for PC mouse.
 * Returns nonzero when a new mouse state has been completed.
 */
static int
ParsePC(int byte)
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


/*
 * Input routine for Microsoft mouse.
 * Returns nonzero when a new mouse state has been completed.
 */
static int
ParseMS(int byte)
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

#if TEST
main()
{
	COORD 	x, y;
	BUTTON	b;

	if(MOU_Open(0) < 0)
		printf("open failed\n");
	while(1) {
		if(MOU_Read(&x, &y, &b) == 1) {
			printf("%d,%d,%d\n", x, y, b);
		}
	}
}
#endif
