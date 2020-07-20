/*
 * Demonstration program for mini-X graphics.
 */
#include <stdio.h>
#include <stdlib.h>
#include "nano-X.h"

/*
 * Definitions to make it easy to define cursors
 */
#define	_	((unsigned) 0)		/* off bits */
#define	X	((unsigned) 1)		/* on bits */
#define	MASK(a,b,c,d,e,f,g) \
	(((((((((((((a * 2) + b) * 2) + c) * 2) + d) * 2) \
	+ e) * 2) + f) * 2) + g) << 9)

#define	W2_WIDTH	70
#define	W2_HEIGHT	40


static	GR_WINDOW_ID	w1;		/* id for large window */
static	GR_WINDOW_ID	w2;		/* id for small window */
static	GR_WINDOW_ID	w3;		/* id for third window */
static	GR_WINDOW_ID	w4;		/* id for grabbable window */
static	GR_WINDOW_ID	w5;		/* id for testing enter/exit window */
static	GR_GC_ID	gc1;		/* graphics context for text */
static	GR_GC_ID	gc2;		/* graphics context for rectangle */
static	GR_GC_ID	gc3;		/* graphics context for circles */
static	GR_GC_ID	gc4;		/* graphics context for lines */
static	GR_COORD	begxpos;	/* beginning x position */
static	GR_COORD	xpos;		/* x position for text drawing */
static	GR_COORD	ypos;		/* y position for text drawing */
static	GR_COORD	linexpos;	/* x position for line drawing */
static	GR_COORD	lineypos;	/* y position for line drawing */
static	GR_COORD	xorxpos;	/* x position for xor line */
static	GR_COORD	xorypos;	/* y position for xor line */
static	GR_BOOL		lineok;		/* ok to draw line */
static	GR_SCREEN_INFO	si;		/* information about screen */

void do_buttondown();
void do_buttonup();
void do_motion();
void do_keystroke();
void do_exposure();
void do_focusin();
void do_focusout();
void do_enter();
void do_exit();
void do_idle();
void errorcatcher();			/* routine to handle errors */

int
main(int argc,char **argv)
{
	GR_EVENT	event;		/* current event */
	GR_BITMAP	bitmap1fg[7];	/* bitmaps for first cursor */
	GR_BITMAP	bitmap1bg[7];
	GR_BITMAP	bitmap2fg[7];	/* bitmaps for second cursor */
	GR_BITMAP	bitmap2bg[7];

	if (GrOpen() < 0) {
		fprintf(stderr, "cannot open graphics\n");
		exit(1);
	}
	
	GrGetScreenInfo(&si);

	GrSetErrorHandler(errorcatcher);

	w1 = GrNewWindow(GR_ROOT_WINDOW_ID, 100, 50, si.cols - 120,
		si.rows - 60, 1, BROWN, WHITE);
	w2 = GrNewWindow(GR_ROOT_WINDOW_ID, 6, 6, W2_WIDTH, W2_HEIGHT, 2, GREEN,
		WHITE);
	w3 = GrNewWindow(GR_ROOT_WINDOW_ID, 250, 30, 80, 100, 1, LTGRAY,
		GREEN);
	w4 = GrNewWindow(GR_ROOT_WINDOW_ID, 350, 20, 200, 150, 5, BLACK, WHITE);
	w5 = GrNewWindow(GR_ROOT_WINDOW_ID, 11, 143, 209, 100, 1, BLUE, GREEN);

	GrSelectEvents(w1, GR_EVENT_MASK_BUTTON_DOWN |
		GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_EXPOSURE |
		GR_EVENT_MASK_FOCUS_IN | GR_EVENT_MASK_FOCUS_OUT);
	GrSelectEvents(w2, GR_EVENT_MASK_BUTTON_UP);
	GrSelectEvents(w3, GR_EVENT_MASK_BUTTON_DOWN |
		GR_EVENT_MASK_MOUSE_MOTION);
	GrSelectEvents(w4, GR_EVENT_MASK_BUTTON_DOWN |
		GR_EVENT_MASK_BUTTON_UP | GR_EVENT_MASK_MOUSE_POSITION |
		GR_EVENT_MASK_KEY_DOWN);
	GrSelectEvents(w5, GR_EVENT_MASK_MOUSE_ENTER |
		GR_EVENT_MASK_MOUSE_EXIT);
	GrSelectEvents(GR_ROOT_WINDOW_ID, GR_EVENT_MASK_BUTTON_DOWN);

	GrMapWindow(w1);
	GrMapWindow(w2);
	GrMapWindow(w3);
	GrMapWindow(w4);
	GrMapWindow(w5);

	gc1 = GrNewGC();
	gc2 = GrNewGC();
	gc3 = GrNewGC();
	gc4 = GrNewGC();

	GrSetGCForeground(gc1, RED);
	GrSetGCBackground(gc1, BROWN);
	GrSetGCForeground(gc2, MAGENTA);
	GrSetGCMode(gc4, GR_MODE_XOR);

	bitmap1fg[0] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[1] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[2] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[3] = MASK(X,X,X,X,X,X,X);
	bitmap1fg[4] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[5] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[6] = MASK(_,_,_,X,_,_,_);

	bitmap1bg[0] = MASK(_,_,X,X,X,_,_);
	bitmap1bg[1] = MASK(_,_,X,X,X,_,_);
	bitmap1bg[2] = MASK(X,X,X,X,X,X,X);
	bitmap1bg[3] = MASK(X,X,X,X,X,X,X);
	bitmap1bg[4] = MASK(X,X,X,X,X,X,X);
	bitmap1bg[5] = MASK(_,_,X,X,X,_,_);
	bitmap1bg[6] = MASK(_,_,X,X,X,_,_);

	bitmap2fg[0] = MASK(_,_,X,X,X,_,_);
	bitmap2fg[1] = MASK(_,X,_,_,_,X,_);
	bitmap2fg[2] = MASK(X,_,_,_,_,_,X);
	bitmap2fg[3] = MASK(X,_,_,_,_,_,X);
	bitmap2fg[4] = MASK(_,X,_,_,_,X,_);
	bitmap2fg[5] = MASK(_,_,X,X,X,_,_);

	bitmap2bg[0] = MASK(_,_,X,X,X,_,_);
	bitmap2bg[1] = MASK(_,X,X,X,X,X,_);
	bitmap2bg[2] = MASK(X,X,X,X,X,X,X);
	bitmap2bg[3] = MASK(X,X,X,X,X,X,X);
	bitmap2bg[4] = MASK(_,X,X,X,X,X,_);
	bitmap2bg[5] = MASK(_,_,X,X,X,_,_);

	GrSetCursor(w1, 7, 7, 3, 3, WHITE, BLACK, bitmap1fg, bitmap1bg);
	GrSetCursor(w2, 7, 7, 3, 3, WHITE, BLACK, bitmap2fg, bitmap2bg);

	GrRect(GR_ROOT_WINDOW_ID, gc1, 0, 0, si.cols, si.rows);


	while (1) {
		GrCheckNextEvent(&event);

		switch (event.type) {
			case GR_EVENT_TYPE_BUTTON_DOWN:
				do_buttondown(&event.button);
				break;

			case GR_EVENT_TYPE_BUTTON_UP:
				do_buttonup(&event.button);
				break;

			case GR_EVENT_TYPE_MOUSE_POSITION:
			case GR_EVENT_TYPE_MOUSE_MOTION:
				do_motion(&event.mouse);
				break;

			case GR_EVENT_TYPE_KEY_DOWN:
				do_keystroke(&event.keystroke);
				break;

			case GR_EVENT_TYPE_EXPOSURE:
				do_exposure(&event.exposure);
				break;

			case GR_EVENT_TYPE_FOCUS_IN:
				do_focusin(&event.general);
				break;

			case GR_EVENT_TYPE_FOCUS_OUT:
				do_focusout(&event.general);
				break;

			case GR_EVENT_TYPE_MOUSE_ENTER:
				do_enter(&event.general);
				break;

			case GR_EVENT_TYPE_MOUSE_EXIT:
				do_exit(&event.general);
				break;

			case GR_EVENT_TYPE_NONE:
				do_idle();
				break;
		}
	}
}


static PIXELVAL	intable[W2_WIDTH * W2_HEIGHT];
static PIXELVAL	outtable[W2_WIDTH * W2_HEIGHT * 4];
/*
 * Here when a button is pressed.
 */
void
do_buttondown(bp)
	GR_EVENT_BUTTON	*bp;
{
	PIXELVAL	*inp;
	PIXELVAL	*outp;
	PIXELVAL	*oldinp;
	GR_COORD	row;
	GR_COORD	col;

	/*static int xx = 100;
	static int yy = 50;*/

	if (bp->wid == w3) {
		GrRaiseWindow(w3);
		GrReadArea(w2, 0, 0, W2_WIDTH, W2_HEIGHT, intable);
		inp = intable;
		outp = outtable;
		for (row = 0; row < W2_HEIGHT; row++) {
			oldinp = inp;
			for (col = 0; col < W2_WIDTH; col++) {
				*outp++ = *inp;
				*outp++ = *inp++;
			}
			inp = oldinp;
			for (col = 0; col < W2_WIDTH; col++) {
				*outp++ = *inp;
				*outp++ = *inp++;
			}
		}
		GrArea(w1, gc1, 0, 0, W2_WIDTH * 2, W2_HEIGHT * 2, outtable);
		return;
	}

	if (bp->wid == w4) {
		GrRaiseWindow(w4);
		linexpos = bp->x;
		lineypos = bp->y;
		xorxpos = bp->x;
		xorypos = bp->y;
		GrLine(w4, gc4, xorxpos, xorypos, linexpos, lineypos);
		lineok = GR_TRUE;
		return;
	}

	if (bp->wid != w1) {
		/*
		 * Cause a fatal error for testing if more than one
		 * button is pressed.
		 */
		if ((bp->buttons & -((int) bp->buttons)) != bp->buttons)
			GrClearWindow(-1, 0);
		return;
	}

	GrRaiseWindow(w1);
	/*GrMoveWindow(w1, ++xx, yy);*/

	if (bp->buttons & GR_BUTTON_L) {
		GrClearWindow(w1, GR_TRUE);
		return;
	}

	begxpos = bp->x;
	xpos = bp->x;
	ypos = bp->y;
}


/*
 * Here when a button is released.
 */
void
do_buttonup(bp)
	GR_EVENT_BUTTON	*bp;
{
	if (bp->wid == w4) {
		if (lineok) {
			GrLine(w4, gc4, xorxpos, xorypos, linexpos, lineypos);
			GrLine(w4, gc3, bp->x, bp->y, linexpos, lineypos);
		}
		lineok = GR_FALSE;
		return;
	}

	if (bp->wid == w2) {
		GrClose();
		exit(0);
	}
}


/*
 * Here when the mouse has a motion event.
 */
void
do_motion(mp)
	GR_EVENT_MOUSE	*mp;
{
	if (mp->wid == w4) {
		if (lineok) {
			GrLine(w4, gc4, xorxpos, xorypos, linexpos, lineypos);
			xorxpos = mp->x;
			xorypos = mp->y;
			GrLine(w4, gc4, xorxpos, xorypos, linexpos, lineypos);
		}
		return;
	}

	if (mp->wid == w3) {
		GrPoint(w3, gc3, mp->x, mp->y);
		return;
	}
}


/*
 * Here when a keyboard press occurs.
 */
void
do_keystroke(kp)
	GR_EVENT_KEYSTROKE	*kp;
{
	GR_SIZE		width;		/* width of character */
	GR_SIZE		height;		/* height of character */
	GR_SIZE		base;		/* height of baseline */

	if (kp->wid == w4) {
		if (lineok) {
			GrLine(w4, gc4, xorxpos, xorypos, linexpos, lineypos);
			lineok = GR_FALSE;
		}
		return;
	}

	GrGetGCTextSize(gc1, &kp->ch, 1, &width, &height, &base);
	if ((kp->ch == '\r') || (kp->ch == '\n')) {
		xpos = begxpos;
		ypos += height;
		return;
	}
	if (kp->ch == '\b') {		/* assumes fixed width font!! */
		if (xpos <= begxpos)
			return;
		xpos -= width;
		GrSetGCForeground(gc3, BROWN);
		GrFillRect(w1, gc3, xpos, ypos - height + base + 1,
			width, height);
		return;
	}
	GrText(w1, gc1, xpos, ypos + base, &kp->ch, 1);
	xpos += width;
}


/*
 * Here when an exposure event occurs.
 */
void
do_exposure(ep)
	GR_EVENT_EXPOSURE	*ep;
{
	GR_POINT	points[3];

	if (ep->wid != w1)
		return;
	points[0].x = 311;
	points[0].y = 119;
	points[1].x = 350;
	points[1].y = 270;
	points[2].x = 247;
	points[2].y = 147;

	GrFillRect(w1, gc2, 50, 50, 150, 200);
	GrFillPoly(w1, gc2, 3, points);
}


/*
 * Here when a focus in event occurs.
 */
void
do_focusin(gp)
	GR_EVENT_GENERAL	*gp;
{
	if (gp->wid != w1)
		return;
	GrSetBorderColor(w1, WHITE);
}

/*
 * Here when a focus out event occurs.
 */
void
do_focusout(gp)
	GR_EVENT_GENERAL	*gp;
{
	if (gp->wid != w1)
		return;
	GrSetBorderColor(w1, GRAY);
}


/*
 * Here when a enter window event occurs.
 */
void
do_enter(gp)
	GR_EVENT_GENERAL	*gp;
{
	if (gp->wid != w5)
		return;
	GrSetBorderColor(w5, WHITE);
	GrRaiseWindow(w5);
}


/*
 * Here when a exit window event occurs.
 */
void
do_exit(gp)
	GR_EVENT_GENERAL	*gp;
{
	if (gp->wid != w5)
		return;
	GrSetBorderColor(w5, GREEN);
	GrLowerWindow(w5);
}


/*
 * Here to do an idle task when nothing else is happening.
 * Just draw a randomly colored filled circle in the small window.
 */
void
do_idle()
{
	GR_COORD	x;
	GR_COORD	y;
	GR_SIZE		rx;
	GR_SIZE		ry;
	GR_COLOR	color;

	x = rand() % 70;
	y = rand() % 40;
	rx = (rand() % 10) + 5;
	ry = (rx * si.ydpcm) / si.xdpcm;	/* make it appear circular */
	
	color = rand() % si.ncolors;

	GrSetGCForeground(gc3, PALINDEX(color));
	GrFillEllipse(w2, gc3, x, y, rx, ry);	
}


/*
 * Here on an unrecoverable error.
 */
void
errorcatcher(code, name, id)
	GR_ERROR	code;		/* error code */
	GR_FUNC_NAME	name;		/* function name which failed */
	GR_ID		id;		/* resource id */
{
	GrClose();
	fprintf(stderr, "DEMO ERROR: code %d, function %s, resource id %d\n",
		code, name, id);
	exit(1);
}
