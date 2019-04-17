/*
 * Copyright (c) 1999, 2000, 2002, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 1991 David I. Bell
 *
 * UNIX Serial Port Mouse Driver
 * 
 * This driver opens a serial port directly, and interprets serial data.
 * Microsoft, PC, Logitech and PS/2 mice are supported.
 * The PS/2 mouse is supported by using the /dev/psaux device.
 *
 * The following environment variables control the mouse type expected
 * and the serial port to open.
 *
 * Environment Var	Default		Allowed
 * MOUSE_TYPE		pc		ms, pc, logi, ps2
 * MOUSE_PORT		/dev/ttyS1	any serial port or /dev/psaux
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
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

#if ELKS /* add "-serial msmouse" to qemu command line */
#define	MOUSE_PORT	"/dev/ttyS0"
#else
/* default mouse tty port: /dev/psaux or /dev/ttyS1 */
#define MOUSE_PORT	"/dev/mouse"
/*#define MOUSE_PORT	"/dev/psaux"*/
/*#define MOUSE_PORT	"/dev/ttyS1"*/
#endif

/* default mouse type: ms, pc, logi, or ps2 */
#define	MOUSE_TYPE	"ms"
//#define MOUSE_TYPE	"ps2"
//#define MOUSE_TYPE	"pc"


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

#define PS2_CTRL_BYTE		0x08
#define PS2_LEFT_BUTTON		1
#define PS2_RIGHT_BUTTON	2

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
static int  	ParsePS2(int);		/* routine to interpret PS/2 mouse */

MOUSEDEVICE mousedev = {
	MOU_Open,
	MOU_Close,
	MOU_GetButtonInfo,
	MOU_GetDefaultAccel,
	MOU_Read,
#if _MINIX
	MOU_Poll
#else
	NULL
#endif
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
	} else if (strcmp(type, "ps2") == 0) {
		/* PS/2 mouse*/
		left = PS2_LEFT_BUTTON;
		right = PS2_RIGHT_BUTTON;
		middle = 0;
		parse = ParsePS2;
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
	/*
	 * Note we don't check success for the tcget/setattr calls,
	 * some kernels don't support them for certain devices
	 * (like /dev/psaux).
	 */

	/* set rawmode serial port using termios*/
	tcgetattr(mouse_fd, &termios);

	/* These functions appear to be broken in ELKS Dev86 */
	if(cfgetispeed(&termios) != B1200)
		cfsetispeed(&termios, B1200);
#if _MINIX
	if(cfgetospeed(&termios) != B1200)
		cfsetospeed(&termios, B1200);
#endif

#if !_MINIX
	termios.c_cflag &= ~CBAUD;
	termios.c_cflag |= B1200;
#endif
	termios.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
	termios.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON | BRKINT | IGNBRK);
	termios.c_cflag &= ~(CSIZE | PARENB);
	/* MS and logi mice use 7 bits*/
	if (!strcmp(type, "ps2") || !strcmp(type, "pc"))
		termios.c_cflag |= CS8;
	else
		termios.c_cflag |= CS7;
	termios.c_cc[VMIN] = 0;
	termios.c_cc[VTIME] = 0;

	tcsetattr(mouse_fd, TCSAFLUSH, &termios);
#endif /* TERMIOS*/

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
	int i;

	/*
	 * If there are no more bytes left, then read some more,
	 * waiting for them to arrive.  On a signal or a non-blocking
	 * error, return saying there is no new state available yet.
	 */
	if (nbytes <= 0) {
		bp = buffer;
		nbytes = read(mouse_fd, bp, MAX_BYTES);
		printf("nbytes=%d,");
		for (i=0;i<10;i++) printf("%X,",buffer[i]);
		printf("\n");
		if (nbytes < 0) {
			if (errno == EINTR || errno == EAGAIN)
				return 0;
#if _MINIX
			return 0;
#else
			return -1;
#endif
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
			//printf("x:%d,y:%d,b:%Xd\n", xd, yd, b);
			return 1;
		}
	}
	printf("x:%d,y:%d,b:%Xd\n", xd, yd, b);
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
  //byte = byte & BOTTOM_SIX_BITS; /* clear upper two bits */
  
	switch (state) {
		case IDLE:
		  printf("idle:%X,",byte);
			if (byte & SIXTH_BIT) {
				buttons = (byte >> 4) & BOTTOM_TWO_BITS;
				yd = ((byte & THIRD_FOURTH_BITS) << 4);
				xd = ((byte & BOTTOM_TWO_BITS) << 6);
				state = XADD;
			}
			break;

		case XADD:
		  printf("xadd:%X,",byte);
			xd |= (byte & BOTTOM_SIX_BITS);
			state = YADD;
			break;

		case YADD:
		  printf("yadd:%X\n",byte);
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

/*
 * Input routine for PS/2 mouse.
 * Returns nonzero when a new mouse state has been completed.
 */
static int
ParsePS2(int byte)
{
	switch (state) {
		case IDLE:
			if (byte & PS2_CTRL_BYTE) {
				buttons = byte & 
					(PS2_LEFT_BUTTON|PS2_RIGHT_BUTTON);
				state = XSET;
			}
			break;

		case XSET:
			if(byte > 127)
				byte -= 256;
			xd = byte;
			state = YSET;
			break;

		case YSET:
			if(byte > 127)
				byte -= 256;
			yd = -byte;
			state = IDLE;
			return 1;
	}
	return 0;
}

static int
MOU_Poll(void)
{
	return 1;	/* used by _MINIX only*/
}

/*  #define TEST 1  */
#if TEST
main()
{
	COORD x, y, z;
	BUTTON	b;

	printf("Open Mouse\n");
	if( MOU_Open(0) < 0)
		printf("open failed mouse\n" );

	while(1) 
	{
		if(MOU_Read(&x, &y, &z, &b) == 1) 
		{
	     		printf("%d,%d,%d\n", x, y, b);
		}
	}
}
#endif
