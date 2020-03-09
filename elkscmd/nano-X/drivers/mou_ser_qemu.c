/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * UNIX Serial Port Mouse Driver for Qemu serial line - Georg Potthast
 *
 * This driver opens a serial port directly, and interprets serial data.
 * Microsoft mice are supported.
 *
 * The following environment variables control the mouse type expected
 * and the serial port to open.
 *
 * Environment Var	Default		Allowed
 * MOUSE_TYPE		ms		ms
 * MOUSE_PORT		/dev/ttyS0	any serial port (for ELKS)
 */

/*
The serial microsoft mouse driver of Qemu as it is working is absolutely not compatible
with a microsoft mouse and rather seems to be code taken from some touchpad. The code
in Qemu looks ok though. This is how it currently works:

Each byte has bit six (counting from zero) set which needs to be cleared first. A left
button click causes to bit 5 to be set and the right button causes bit 4 to be set. Bits
zero to three contain the mouse movement. A zero means the mouse moved down, a 0X0F means
the mouse moved up, a 0x0C means the mouse moved right and 3 means the mouse moved left.
I translated these movements into 5 points positive and negative as required. Plus using
a high resolution.

When a button is clicked, the byte contains no movement data. When the mouse is moved with
the button down, the button bit remains set while the lower four bits contain the usual
movement data. When the button is released, a zero byte is send by the mouse. Since a zero
byte can also mean mouse down, this byte means a release when the button bit had been set
in the byte before it. In that case this byte contains no movement data and just indicates
a button release.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "../device.h"

#define TERMIOS		1	/* set to use termios serial port control*/
#define SGTTY		0	/* set to use sgtty serial port control*/

#define	SCALE		18	/* default scaling factor for acceleration */
#define	THRESH		30	/* default threshhold for acceleration */
#define MVALUE		5	/* amount of mouse movement */

#if TERMIOS
#include <termios.h>
#endif
#if SGTTY
#include <sgtty.h>
#endif

/* default settings*/
#if ELKS /* add "-serial msmouse" to qemu command line */
#define	MOUSE_PORT	"/dev/ttyS0"	/* default mouse tty port */
#else
#define	MOUSE_PORT	"/dev/ttyS1"	/* default mouse tty port */
#endif
#define	MOUSE_TYPE	"ms"		/* default mouse type ("ms" or "pc") */
#define MAX_BYTES	128		/* number of bytes for buffer */

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

/* local routines*/
static int  	MOU_Open(MOUSEDEVICE *pmd);
static void 	MOU_Close(void);
static BUTTON  	MOU_GetButtonInfo(void);
static void	MOU_GetDefaultAccel(int *pscale,int *pthresh);
static int  	MOU_Read(COORD *dx, COORD *dy, COORD *dz, BUTTON *bptr);

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
		//parse = ParsePC;
	} else if (strcmp(type, "ms") == 0) {
		/* microsoft mouse*/
		left = MS_LEFT_BUTTON;
		right = MS_RIGHT_BUTTON;
		middle = 0;
		//parse = ParseMS;
	} else
		return -1;

	/* open mouse port*/
	mouse_fd = open(port, O_NONBLOCK);
	if (mouse_fd < 0) {
		fprintf(stderr,
			"Error %d opening serial mouse type %s on port %s.\n",
			errno, type, port);
		printf("Cannot open mouse on %s\n",port);
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
	termios.c_cflag |= CS7;
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
	int b, byte;
	int i;
	static unsigned int buttonpressed;

	bp = buffer;
	nbytes = read(mouse_fd, bp, 1);
//printf("\nnbytes=%d,",nbytes);
//printf("buffer:%X,",buffer[0]);

	if (nbytes < 0) {
		if (errno == EINTR || errno == EAGAIN)
			return 0;
		return -1;
	}
	byte=buffer[0];
	xd=0;
	yd=0;
//printf("byte:%X,",byte);
	if ((byte == 0x40) && (buttonpressed > 0)) { //button release
	  b=0;
	  buttonpressed=0;
	  *dx=xd;
	  *dy=yd;
//printf("button released\n");
	  return 1;
	}

	byte &= 0xBF; //clear bit 6
	b = 0;
//printf("bytecleared:%X,",byte);
	if(byte & 0x20) {
	  b |= LBUTTON;
	} else 	if(byte & 0x10) {
	  b |= RBUTTON;
	} else {
	  b = 0;
	  buttonpressed = 0;
	}
	  if ((b>0) && (buttonpressed == 0)) {
	    buttonpressed = 1; //first down
	  } else if ((b>0) && (buttonpressed == 1)) {
	    buttonpressed =2; //dragging
	  }

	*bptr = b;
//printf("b:%X, bpressed:%X, ",b,buttonpressed);
	byte &= 0xCF; //clear button bits
//printf("bytecleared2:%X,",byte);
	if ((byte == 0) && (buttonpressed == 1)){ //no movement data at first click
	  *dx=xd;
	  *dy=yd;
//printf("x:%X,y:%X,b:%X-nm\n",xd,yd,b);
	  return 1;
	}

	switch (byte) {
	  case 0:
	    yd = MVALUE; /*up*/
	    break;
	  case 15: /*0xF*/
	    yd = -MVALUE; /*down*/
	    break;
	  case 12: /*0xC*/
	    xd = MVALUE; /*right*/
	    break;
	  case 3:
	    xd = -MVALUE; /*left*/
	    break;
	  default:
	    xd = 0;
	    yd = 0;
//printf("default\n");
	}
	*dx=xd;
	*dy=yd;
	if ((xd+yd+b)==0) return 0; //no valid data
//printf("x:%X,y:%X,b:%X\n",xd,yd,b);
	return 1; //ok
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
