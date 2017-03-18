/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Copyright (c) 1999 Alex Holden
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * GPM Mouse Driver
 *
 * Rewritten to understand the Logitech Mouseman protocol which GPM
 * produces on /dev/gpmdata when in repeater mode. Remember to start
 * GPM with the -R flag or it won't work. (gpm -R -t ps2)
 *
 */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "../device.h"

#define	SCALE		3	/* default scaling factor for acceleration */
#define	THRESH		5	/* default threshhold for acceleration */

#define GPM_DEV_FILE	"/dev/gpmdata"

static int  	GPM_Open(MOUSEDEVICE *pmd);
static void 	GPM_Close(void);
static BUTTON  	GPM_GetButtonInfo(void);
static void	MOU_GetDefaultAccel(int *pscale,int *pthresh);
static int  	GPM_Read(COORD *dx, COORD *dy, COORD *dz, BUTTON *bp);

MOUSEDEVICE mousedev = {
	GPM_Open,
	GPM_Close,
	GPM_GetButtonInfo,
	MOU_GetDefaultAccel,
	GPM_Read,
	NULL
};

static int mouse_fd;

/*
 * Open up the mouse device.
 * Returns the fd if successful, or negative if unsuccessful.
 */
static int
GPM_Open(MOUSEDEVICE *pmd)
{
	mouse_fd = open(GPM_DEV_FILE, O_NONBLOCK);
	if (mouse_fd < 0)
		return -1;
	return mouse_fd;
}

/*
 * Close the mouse device.
 */
static void
GPM_Close(void)
{
	if (mouse_fd > 0)
		close(mouse_fd);
	mouse_fd = 0;
}

/*
 * Get mouse buttons supported
 */
static BUTTON
GPM_GetButtonInfo(void)
{
	return LBUTTON | MBUTTON | RBUTTON;
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
GPM_Read(COORD *dx, COORD *dy, COORD *dz, BUTTON *bp)
{
	static unsigned char buf[5];
	static int nbytes;
	int n;

	while((n = read(mouse_fd, &buf[nbytes], 5 - nbytes))) {
		if(n < 0) {
			if ((errno == EINTR) || (errno == EAGAIN))
				return 0;
			else return -1;
		}

		nbytes += n;

		if(nbytes == 5) {
			/* button data matches defines, no conversion*/
			*bp = (~buf[0]) & 0x07;
			*dx = (signed char)(buf[1]) + (signed char)(buf[3]);
			*dy = -((signed char)(buf[2]) + (signed char)(buf[4]));
			*dz = 0;
			nbytes = 0;
			return 1;
		}
	}
	return 0;
}
