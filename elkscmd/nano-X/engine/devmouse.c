/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Copyright (C) 1999 Bradley D. LaRonde (brad@ltc.com)
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Device-independent top level mouse and cursor routines
 *
 * Reads data from mouse driver and tracks real position on the screen.
 * Intersection detection for cursor with auto removal
 *
 * Bradley D. LaRonde added absolute coordinates and z (pen pressure) Oct-1999
 */
#include "device.h"

static COORD	xpos;		/* current x position of mouse */
static COORD	ypos;		/* current y position of mouse */
static COORD	minx;		/* minimum allowed x position */
static COORD	maxx;		/* maximum allowed x position */
static COORD	miny;		/* minimum allowed y position */
static COORD	maxy;		/* maximum allowed y position */
static int	scale;		/* acceleration scale factor */
static int	thresh;		/* acceleration threshhold */
static BUTTON	buttons;	/* current state of buttons */
static BOOL	changed;	/* mouse state has changed */

static COORD 	curminx;	/* minimum x value of cursor */
static COORD 	curminy;	/* minimum y value of cursor */
static COORD 	curmaxx;	/* maximum x value of cursor */
static COORD 	curmaxy;	/* maximum y value of cursor */
static BOOL	curvisible;	/* TRUE if cursor is visible*/
static BOOL 	curneedsrestore;/* cursor needs restoration after drawing*/
static COORD 	cursavx;	/* saved cursor location*/
static COORD 	cursavy;
static COORD	cursavx2;
static COORD	cursavy2;
static PIXELVAL curfg;		/* foreground color of cursor */
static PIXELVAL curbg;		/* background color of cursor */
static PIXELVAL cursavbits[MAX_CURSOR_SIZE * MAX_CURSOR_SIZE];
static IMAGEBITS cursormask[MAX_CURSOR_SIZE];
static IMAGEBITS cursorcolor[MAX_CURSOR_SIZE];

extern MODE gr_mode;

/*
 * Initialize the mouse.
 * This sets its position to (0, 0) with no boundaries and no buttons pressed.
 * Returns < 0 on error, or mouse fd on success
 */
int
GdOpenMouse(void)
{
	int fd;

	if ((fd = mousedev.Open(&mousedev)) == 1) /* -2 is mou_nul.c */
		return -1;

	/* get default acceleration settings*/
	mousedev.GetDefaultAccel(&scale, &thresh);

	/* init mouse position info*/
	buttons = 0;
	xpos = 0;
	ypos = 0;
	minx = COORD_MIN;
	miny = COORD_MIN;
	maxx = COORD_MAX;
	maxy = COORD_MAX;
	changed = TRUE;

	/* init cursor position and size info*/
	curvisible = FALSE;
	curneedsrestore = FALSE;
	curminx = minx;
	curminy = miny;
	curmaxx = curminx + MAX_CURSOR_SIZE - 1;
	curmaxy = curminy + MAX_CURSOR_SIZE - 1;

	return fd;
}

/*
 * Terminate the use of the mouse.
 */
void
GdCloseMouse(void)
{
	mousedev.Close();
}

void
GdGetButtonInfo(BUTTON *buttons)
{
	*buttons = mousedev.GetButtonInfo();
}

/*
 * Restrict the coordinates of the mouse to the specified coordinates.
 */
void
GdRestrictMouse(COORD newminx, COORD newminy, COORD newmaxx, COORD newmaxy)
{
	minx = newminx;
	miny = newminy;
	maxx = newmaxx;
	maxy = newmaxy;
	GdMoveMouse(xpos, ypos);
}

/*
 * Set the acceleration threshhold and scale factors.
 * Acceleration makes the cursor move further for faster movements.
 * Basically, at mouse speeds above the threshold, the excess distance
 * moved is multiplied by the scale factor.  For example, with a threshhold
 * of 5 and a scale of 3, the following gives examples of the original and
 * modified mouse movements:
 *	input:		0	4	5	6	9	20
 *	output:		0	4	5	8	17	50
 */
void
GdSetAccelMouse(int newthresh, int newscale)
{
	if (newthresh < 0)
		newthresh = 0;
	if (newscale < 0)
		newscale = 0;
	thresh = newthresh;
	scale = newscale;
}

/*
 * Move the mouse to the specified coordinates.
 * The location is limited by the current mouse coordinate restrictions.
 */
void
GdMoveMouse(COORD newx, COORD newy)
{
	if (newx < minx)
		newx = minx;
	if (newx > maxx)
		newx = maxx;
	if (newy < miny)
		newy = miny;
	if (newy > maxy)
		newy = maxy;
	if (newx == xpos && newy == ypos)
		return;

	changed = TRUE;
	xpos = newx;
	ypos = newy;
}

/*
 * Read the current location and button states of the mouse.
 * Returns -1 on read error.
 * Returns 0 if no new data is available from the mouse driver,
 * or if the new data shows no change in button state or position.
 * Returns 1 if the mouse driver tells us a changed button state
 * or position. Button state and position are always both returned,
 * even if only one or the other changes.
 * Do not block.
 */
int
GdReadMouse(COORD *px, COORD *py, BUTTON *pb)
{
	COORD	x, y, z;
	BUTTON	newbuttons;	/* new button state */
	int	sign;		/* sign of change */
	int	status;		/* status of reading mouse */

	*px = xpos;
	*py = ypos;
	*pb = buttons;

	if (changed) {
		changed = FALSE;
		return 1;
	}

	/* read the mouse position */
	status = mousedev.Read(&x, &y, &z, &newbuttons);
	if (status < 0)
		return -1;

	/* no new info from the mouse driver? */
	if (status == 0)
		return 0;

	/* has the button state changed? */
	if (buttons != newbuttons) {
		changed = TRUE;
		buttons = newbuttons;
	}

	/* depending on the kind of data that we have */
	switch(status) {
	case 1:	/* relative position change reported, figure new position */
		sign = 1;
		if (x < 0) {
			sign = -1;
			x = -x;
		}
		if (x > thresh)
			x = thresh + (x - thresh) * scale;
		x *= sign;

		sign = 1;
		if (y < 0) {
			sign = -1;
			y = -y;
		}
		if (y > thresh)
			y = thresh + (y - thresh) * scale;
		y *= sign;

		GdMoveMouse(xpos + x, ypos + y);
		break;

	case 2:	/* absolute position reported */
		GdMoveMouse(x, y);
		break;

	case 3:	/* only button data is available */
		break;
	}

	/* didn't anything change? */
	if (!changed)
		return 0;

	/* report new mouse data */
	changed = FALSE;
	*px = xpos;
	*py = ypos;
	*pb = buttons;
	return 1;
}

/*
 * Set the cursor position.
 */
void
GdMoveCursor(COORD newx, COORD newy)
{
	COORD shiftx;
	COORD shifty;

	shiftx = newx - curminx;
	shifty = newy - curminy;
	if(shiftx == 0 && shifty == 0)
		return;
	curminx += shiftx;
	curmaxx += shiftx;
	curminy += shifty;
	curmaxy += shifty;

	/* Restore the screen under the mouse pointer*/
	GdHideCursor();

	/* Draw the new pointer*/
	GdShowCursor();
}

/*
 * Set the cursor size and bitmaps.
 */
void
GdSetCursor(PSWCURSOR pcursor)
{
	BOOL	savevisible;
	int	bytes;

	savevisible = GdHideCursor();
	curmaxx = curminx + pcursor->width - 1;
	curmaxy = curminy + pcursor->height - 1;

	curfg = GdFindColor(pcursor->fgcolor);
	curbg = GdFindColor(pcursor->bgcolor);
	bytes = IMAGE_WORDS(pcursor->width) * pcursor->height
			* sizeof(IMAGEBITS);
	memcpy(cursorcolor, pcursor->image, bytes);
	memcpy(cursormask, pcursor->mask, bytes);

	if (savevisible)
		GdShowCursor();
}


/*
 * Draw the mouse pointer.  Save the screen contents underneath
 * before drawing. Returns previous cursor state.
 */
int
GdShowCursor(void)
{
	COORD 		x;
	COORD 		y;
	PIXELVAL *	saveptr;
	IMAGEBITS *	cursorptr;
	IMAGEBITS *	maskptr;
	IMAGEBITS 	curbit, cbits, mbits;
	PIXELVAL 	oldcolor;
	PIXELVAL 	newcolor;
	MODE 		oldmode;

	if(curvisible)
		return TRUE;
	oldmode = gr_mode;
	gr_mode = MODE_SET;

	saveptr = cursavbits;
	cursavx = curminx;
	cursavy = curminy;
	cursavx2 = curmaxx;
	cursavy2 = curmaxy;
	cursorptr = cursorcolor;
	maskptr = cursormask;

	for (y = curminy; y <= curmaxy; y++) {
		cbits = *cursorptr++;
		mbits = *maskptr++;
		curbit = IMAGE_FIRSTBIT;
		for (x = curminx; x <= curmaxx; x++) {
			if(x >= 0 && x < scrdev.xres &&
			   y >= 0 && y < scrdev.yres) {
				oldcolor = scrdev.ReadPixel(&scrdev, x, y);
				if (curbit & mbits) {
					newcolor = (curbit&cbits)? curbg: curfg;
					if (oldcolor != newcolor)
					       scrdev.DrawPixel(&scrdev, x, y, newcolor);
				}
				*saveptr++ = oldcolor;
			}
			curbit = IMAGE_NEXTBIT(curbit);
		}
	}

	gr_mode = oldmode;
	curvisible = TRUE;
	curneedsrestore = FALSE;
	return FALSE;
}

/*
 * Restore the screen overwritten by the cursor.
 */
int
GdHideCursor(void)
{
	PIXELVAL *	saveptr;
	COORD 		x, y;
	MODE 		oldmode;

	if(!curvisible)
		return FALSE;
	oldmode = gr_mode;
	gr_mode = MODE_SET;

	saveptr = cursavbits;
	for (y = cursavy; y <= cursavy2; y++) {
		for (x = cursavx; x <= cursavx2; x++) {
			if(x >= 0 && x < scrdev.xres &&
			   y >= 0 && y < scrdev.yres) {
				scrdev.DrawPixel(&scrdev, x, y, *saveptr++);
			}
		}
	}
 	gr_mode = oldmode;
	curvisible = FALSE;
	return TRUE;
}

/* Check to see if the mouse pointer is about to be overwritten.
 * If so, then remove the cursor so that the graphics operation
 * works correctly.  If the cursor is removed, then this fact will
 * be remembered and a later call to GdFixCursor will restore it.
 */
void
GdCheckCursor(COORD x1,COORD y1,COORD x2,COORD y2)
{
	COORD temp;

	if(curneedsrestore)
		return;

	if (x1 > x2) {
		temp = x1;
		x1 = x2;
		x2 = temp;
	}
	if (y1 > y2) {
		temp = y1;
		y1 = y2;
		y2 = temp;
	}
	if (x1 > curmaxx || x2 < curminx || y1 > curmaxy || y2 < curminy)
		return;

	GdHideCursor();
	curneedsrestore = 1;
}


/* Redisplay the cursor if it was removed because of a graphics operation. */
void
GdFixCursor(void)
{
	if (curneedsrestore)
		GdShowCursor();
}
