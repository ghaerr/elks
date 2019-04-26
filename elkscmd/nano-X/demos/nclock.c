/* nano-X clock program
 * (C) 1999 Alistair Riddoch <ajr@ecs.soton.ac.uk>
 *
 * This file is distributed under the terms of the GNU GPL version 2 or later
 */

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include "nano-X.h"



unsigned char trig[91] =
	{ 0, 4, 8, 13, 17, 22, 26, 31, 35, 40, 44, 48, 53, 57, 61, 66,
	70, 74, 79, 83, 87, 91, 95, 100, 104, 108, 112, 116, 120, 124, 128,
	131, 135, 139, 143, 146, 150, 154, 157, 161, 164, 167, 171, 174, 177,
	181, 184, 187, 190, 193, 196, 198, 201, 204, 207, 209, 212, 214, 217,
	219, 221, 223, 226, 228, 230, 232, 233, 235, 237, 238, 240, 242, 243,
	244, 246, 247, 248, 249, 250, 251, 252, 252, 253, 254, 254, 255, 255,
	255, 255, 255, 255}; 

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

/* If you need a clock bigger than 200x200 you will need to re-write the trig *
 * to use longs. (Only applies under ELKS I think. */

#define CWIDTH		200		/* Max 200 */
#define CHEIGHT		200		/* Max 200 */


static	GR_WINDOW_ID	w1;		/* id for large window */
static	GR_GC_ID	gc1;		/* graphics context for text */
static	GR_GC_ID	gc2;		/* graphics context for rectangle */
static	GR_GC_ID	gc3;		/* graphics context for rectangle */
static	GR_SCREEN_INFO	si;		/* information about screen */

void do_exposure();
void do_idle();
void errorcatcher();			/* routine to handle errors */

int
main(int argc,char **argv)
{
	GR_EVENT	event;		/* current event */
	GR_BITMAP	bitmap1fg[7];	/* bitmaps for first cursor */
	GR_BITMAP	bitmap1bg[7];

	if (GrOpen() < 0) {
		fprintf(stderr, "cannot open graphics\n");
		exit(1);
	}
	
	GrGetScreenInfo(&si);

	GrSetErrorHandler(errorcatcher);

	w1 = GrNewWindow(GR_ROOT_WINDOW_ID, 100, 50, CWIDTH + 1,
		CHEIGHT + 1, 1, WHITE, BLACK);

	GrSelectEvents(w1, GR_EVENT_MASK_EXPOSURE);


	gc1 = GrNewGC();
	gc2 = GrNewGC();
	gc3 = GrNewGC();

	GrSetGCForeground(gc1, WHITE);
	GrSetGCBackground(gc1, BLACK);
	GrSetGCForeground(gc2, BLACK);
	GrSetGCBackground(gc2, WHITE);
	GrSetGCForeground(gc3, GRAY);

	bitmap1bg[0] = MASK(_,_,X,X,X,_,_);
	bitmap1bg[1] = MASK(_,X,X,X,X,X,_);
	bitmap1bg[2] = MASK(X,X,X,X,X,X,X);
	bitmap1bg[3] = MASK(X,X,X,X,X,X,X);
	bitmap1bg[4] = MASK(X,X,X,X,X,X,X);
	bitmap1bg[5] = MASK(_,X,X,X,X,X,_);
	bitmap1bg[6] = MASK(_,_,X,X,X,_,_);

	bitmap1fg[0] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[1] = MASK(_,X,_,X,_,X,_);
	bitmap1fg[2] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[3] = MASK(X,_,_,X,X,_,X);
	bitmap1fg[4] = MASK(_,_,_,_,_,_,_);
	bitmap1fg[5] = MASK(_,X,_,_,_,X,_);
	bitmap1fg[6] = MASK(_,_,_,X,_,_,_);

	GrSetCursor(w1, 7, 7, 3, 3, WHITE, BLACK, bitmap1fg, bitmap1bg);

	GrFillRect(GR_ROOT_WINDOW_ID, gc3, 0, 0, si.cols, si.rows);

	GrSetGCForeground(gc3, BLACK);
	GrSetGCBackground(gc3, WHITE);
/*	GrSetGCMode(gc3, GR_MODE_XOR); */

	GrMapWindow(w1);

	while (1) {
		GrCheckNextEvent(&event);

		switch (event.type) {
			case GR_EVENT_TYPE_EXPOSURE:
				do_exposure(&event.exposure);
				break;

			case GR_EVENT_TYPE_NONE:
				do_idle();
				break;
		}
	}
}

int bsin(int angle)
{
	if(angle < 91) {
		return trig[angle];
	} else if (angle < 181) {
		return trig[180 - angle];
	} else if (angle < 271) {
		return -trig[angle - 180];
	} else if (angle < 361) {
		return -trig[360 - angle];
	} else {
		return 0;
	}
}

int bcos(int angle)
{
	if(angle < 91) {
		return trig[90 - angle];
	} else if (angle < 181) {
		return -trig[angle - 90];
	} else if (angle < 271) {
		return -trig[270 - angle];
	} else if (angle < 361) {
		return trig[angle - 270];
	} else {
		return 0;
	}
}

/*
 * Here when an exposure event occurs.
 */
void
do_exposure(ep)
	GR_EVENT_EXPOSURE	*ep;
{
	GR_COORD	midx = CWIDTH / 2;
	GR_COORD	midy = CHEIGHT / 2;
	int i, l;

/*	GrFillRect(w1, gc2, 0, 0, CWIDTH, CHEIGHT); */
/*	GrFillEllipse(w1, gc1, midx, midy, midx, midy); */
	GrEllipse(w1, gc2, midx, midy, midx - 1, midy - 1);
	for(i = 0; i < 60; i++) {
		if (i%5 == 0) {
			l = 5;
		} else {
			l = 0;
		}
		GrLine(w1, gc2,
			midx + (((midx - 3) * bsin(i * 6)) >> 8), 
			midy + (((midy - 3) * bcos(i * 6)) >> 8), 
			midx + (((midx - 3 - l) * bsin(i * 6)) >> 8), 
			midy + (((midy - 3 - l) * bcos(i * 6)) >> 8));
	}
	
	
}

void draw_time(int hour, int min, int sec, GR_GC_ID gc )
{
	GR_COORD	midx = CWIDTH / 2;
	GR_COORD	midy = CHEIGHT / 2;
	static int lh = -1, lm = -1, ls = -1;

	GrLine(w1, gc1,
		midx + (((midx - 8 - midx / 10) * bsin(ls)) >> 8), 
		midy - (((midy - 8 - midy / 10) * bcos(ls)) >> 8), 
		midx + (((midx - 8 - midx / 4) * bsin(ls)) >> 8), 
		midy - (((midy - 8 - midx / 4) * bcos(ls)) >> 8));
	GrLine(w1, gc2,
		midx + (((midx - 8 - midx / 10) * bsin(sec)) >> 8), 
		midy - (((midy - 8 - midy / 10) * bcos(sec)) >> 8), 
		midx + (((midx - 8 - midx / 4) * bsin(sec)) >> 8), 
		midy - (((midy - 8 - midx / 4) * bcos(sec)) >> 8));
	if ((lm != min) || (ls == min)) {
		GrLine(w1, gc1,
			midx + (((midx - 8 - midx / 10) * bsin(lm)) >> 8), 
			midy - (((midy - 8 - midy / 10) * bcos(lm)) >> 8), 
			midx + (((midx / 5) * bsin(lm)) >> 8), 
			midy - (((midy / 5) * bcos(lm)) >> 8));
		GrLine(w1, gc2,
			midx + (((midx - 8 - midx / 10) * bsin(min)) >> 8), 
			midy - (((midy - 8 - midy / 10) * bcos(min)) >> 8), 
			midx + (((midx / 5) * bsin(min)) >> 8), 
			midy - (((midy / 5) * bcos(min)) >> 8));
		GrLine(w1, gc1,
			midx + (((midx - 8 - midx / 2) * bsin(lh)) >> 8), 
			midy - (((midy - 8 - midy / 2) * bcos(lh)) >> 8), 
			midx + (((midx / 5) * bsin(lh)) >> 8), 
			midy - (((midy / 5) * bcos(lh)) >> 8));
		GrLine(w1, gc2,
			midx + (((midx - 8 - midx / 2) * bsin(hour)) >> 8), 
			midy - (((midy - 8 - midy / 2) * bcos(hour)) >> 8), 
			midx + (((midx / 5) * bsin(hour)) >> 8), 
			midy - (((midy / 5) * bcos(hour)) >> 8));
	}
	lh = hour;
	lm = min;
	ls = sec;
}


/*
 * Here to do an idle task when nothing else is happening.
 * Just draw a randomly colored filled circle in the small window.
 */
void
do_idle()
{
	struct timeval tv;
	struct timezone tz;
	struct tm * tp;
	time_t now;
	static time_t then;
	int min, hour, sec;

	gettimeofday(&tv, &tz);
	now = tv.tv_sec - (60 * tz.tz_minuteswest);
	if (now == then) {
		return;
	}
	then = now;
	tp = gmtime(&now);
	min = tp->tm_min * 6;
	sec = tp->tm_sec * 6;
	hour = tp->tm_hour;
	if (hour > 12) {
		hour -= 12;
	}
	hour *= 30;
	draw_time(hour, min, sec, gc2);
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
