/*
 * Draw a crude map of the world using mini-X graphics on MINIX.
 * Converted from an Amiga program by Mike Groshart and Bob Dufford.
 * Author: David I. Bell
 *
 * ported to 16 bit systems by Greg Haerr
 */
#include <stdio.h>
#include <string.h>
#include "nano-X.h"

#if MSDOS
#include <fcntl.h>
#endif

#ifndef O_BINARY
#define O_BINARY 	0
#endif

#define	MAPFILE	"/lib/nxworld.map"

#define	SELECTBUTTON	GR_BUTTON_L
#define	COORDBUTTON	GR_BUTTON_R


/*
 * Definitions to use fixed point in place of true floating point.
 */
typedef	long	FLOAT;

#define	SCALE	100		/* fixed point scaling factor */

#define	FFMUL(a,b)	(((FLOAT)(a) * (b) + (SCALE / 2)) / SCALE)
#define	FFDIV(a,b)	(((FLOAT)(a) * SCALE) / (b))
#define	FIMUL(a,b)	((FLOAT)(a) * (b))
#define	FIDIV(a,b)	((FLOAT)(a) / (b))
#define	ITOF(a)		((FLOAT)(a) * SCALE)
#define	FTOI(a)		(((FLOAT)(a) + (SCALE / 2)) / SCALE)


#define	QSPAN	(90L*60*SCALE)	/* equator to pole (90 degrees) */
#define HSPAN	(QSPAN*2)	/* pole to pole (180 degrees) */
#define WSPAN	(QSPAN*4)	/* around equator (360 degrees) */

#define	ABS(n)	(((n) < 0) ? -(n) : (n))


/*
 * Structure of a point in the database file.
 */
typedef struct {
	short	Code;		/* type of point (see code_colors below) */
	short	Lat;		/* latitude in minutes */
	short	Lon;		/* longitude in minutes */
} DBPOINT;


#define POINTSize	sizeof(DBPOINT)
#define PCount		1024		/* number of points to read at once */


/*
 * The following variables are the scaling factors to be used when drawing
 * points.  However, they are larger than the true value by a factor of 60.
 * This is done because without real floating point, their true values are
 * too small to be accurate enough.  I cannot just increase the fixed point
 * precision because that causes overflows.  What a pain!
 */
static	FLOAT		X_Scale;
static	FLOAT		Y_Scale;

/*
 * Other variables.
 */
static	FLOAT		Latitude, Longitude;	/* current center of view */
static	FLOAT		zoom;		/* current zoom scaling factor */

static	FLOAT		latradius;	/* half of view of latitide */
static	FLOAT		longradius;	/* half of view of longitude */
static	FLOAT		viewlong;	/* amount of longitide in view */
static	FLOAT		viewlat;	/* amount of latitude in view */

static	GR_SIZE		mapwidth;	/* width of map in pixels */
static	GR_SIZE		mapheight;	/* height of map in pixels */
static	GR_COORD	mapxorig;	/* one half of map width */
static	GR_COORD	mapyorig;	/* one half of map height */
static	GR_COORD	selectx;	/* x position of current selection */
static	GR_COORD	selecty;	/* y position of current selection */
static	GR_COORD	selectptrx;	/* x position of pointer in selection */
static	GR_COORD	selectptry;	/* y position of pointer in selection */
static	GR_SIZE		selectwidth;	/* width of current selection */
static	GR_SIZE		selectheight;	/* height of current selection */
static	int		selectmode;	/* selection mode */
static	GR_BOOL		selectvisible;	/* TRUE if selection is visible on screen */
static	GR_SIZE		selectxscale;	/* x scaling factor for selection rectangle */
static	GR_SIZE		selectyscale;	/* y scaling factor for selection rectangle */
static	GR_BOOL		coordvisible;	/* TRUE if coordinates are visible on screen */
static	GR_BOOL		coordenabled;	/* TRUE if coordinate display is enabled */
static	GR_COORD	coordx;		/* x position of coordinates */
static	GR_COORD	coordy;		/* y position of coordinates */
static	GR_COORD	ptrx;		/* latest x position of pointer */
static	GR_COORD	ptry;		/* latest y position of pointer */
static	char		coordstring[32];	/* coordinate string */

static	GR_WINDOW_ID	mainwid;	/* main window id */
static	GR_WINDOW_ID	mapwid;		/* window id for map */
static	GR_GC_ID	mapgc;		/* GC used for drawing map */
static	GR_GC_ID	xorgc;		/* GC used for rubber banding */



/*
 * Current selection mode
 */
#define	SELECT_NONE	0
#define	SELECT_SCALE	1
#define	SELECT_MOVE	2

/*
 * Order of color table (indexed by type of point):
 *	unused
 *	continents
 *	countries
 *	unused
 *	USA states
 *	islands
 *	lakes
 *	rivers
 */
static	GR_COLOR	code_colors[] = {
	BLACK, GREEN, RED, BLACK, BROWN, GREEN, BLUE, BLUE
};


static	void	load();
static	void	setzoom();
static	void	checkevent();
static	void	doexposure();
static	void	dobuttondown();
static	void	dobuttonup();
static	void	doposition();
static	void	dokeydown();
static	void	showselection();
static	void	showcoords();
static	void	mintostr();


int
main(argc, argv)
	char	**argv;
{
	GR_SCREEN_INFO	si;

	if (GrOpen() < 0) {
		fprintf(stderr, "Cannot open graphics\n");
		exit(1);
	}
	GrGetScreenInfo(&si);

	mainwid = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, si.cols, si.rows,
		0, BLACK, BLACK);

	mapwidth = si.cols - 2;
	mapheight = si.rows - 2;
	mapxorig = mapwidth / 2;
	mapyorig = mapheight / 2;
	selectxscale = 4;
	selectyscale = 3;
	coordx = 0;
	coordy = si.rows - 1;
	mapwid = GrNewWindow(mainwid, 1, 1, mapwidth, mapheight,
		1, BLACK, WHITE);

	GrSelectEvents(mapwid, GR_EVENT_MASK_EXPOSURE |
		GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP |
		GR_EVENT_MASK_MOUSE_POSITION | GR_EVENT_MASK_KEY_DOWN);

	GrMapWindow(mainwid);
	GrMapWindow(mapwid);

	mapgc = GrNewGC();
	xorgc = GrNewGC();
	GrSetGCMode(xorgc, GR_MODE_XOR);

	Longitude = ITOF(0);
	Latitude = ITOF(0);
	setzoom(ITOF(1));

	while (1)
		checkevent();
}


static void
checkevent()
{
	GR_EVENT	event;

	GrGetNextEvent(&event);
	switch (event.type) {
		case GR_EVENT_TYPE_EXPOSURE:
			doexposure(&event.exposure);
			break;
		case GR_EVENT_TYPE_BUTTON_DOWN:
			dobuttondown(&event.button);
			break;
		case GR_EVENT_TYPE_BUTTON_UP:
			dobuttonup(&event.button);
			break;
		case GR_EVENT_TYPE_MOUSE_POSITION:
			doposition(&event.mouse);
			break;
		case GR_EVENT_TYPE_KEY_DOWN:
			dokeydown(&event.keystroke);
			break;
	}
}


static void
doexposure(ep)
	GR_EVENT_EXPOSURE	*ep;
{
	if (ep->wid != mapwid)
		return;

	GrClearWindow(mapwid, GR_FALSE);
	selectvisible = GR_FALSE;
	coordvisible = GR_FALSE;
	load(MAPFILE);
	showselection(GR_TRUE);
	showcoords(GR_TRUE);
}


static void
dobuttondown(bp)
	GR_EVENT_BUTTON	*bp;
{
	if (bp->wid != mapwid)
		return;

	if (bp->changebuttons & SELECTBUTTON) {
		showselection(GR_FALSE);
		selectx = bp->x;
		selecty = bp->y;
		selectptrx = bp->x;
		selectptry = bp->y;
		selectwidth = 0;
		selectheight = 0;
		selectmode = SELECT_SCALE;
		showselection(GR_TRUE);
	}

	if (bp->changebuttons & COORDBUTTON) {
		showcoords(GR_FALSE);
		ptrx = bp->x;
		ptry = bp->y;
		coordenabled = GR_TRUE;
		showcoords(GR_TRUE);
	}
}


static void
dobuttonup(bp)
	GR_EVENT_BUTTON	*bp;
{
	if (bp->wid != mapwid)
		return;

	if (bp->changebuttons & COORDBUTTON) {
		showcoords(GR_FALSE);
		coordenabled = GR_FALSE;
	}

	if (bp->changebuttons & SELECTBUTTON) {
		showselection(GR_FALSE);
		if (selectmode == SELECT_NONE)
			return;
		selectmode = SELECT_NONE;
		if (selectwidth <= 0)
			return;
		Longitude +=
			FIDIV(FIMUL(viewlong, selectx - mapxorig), mapwidth);
		Latitude -=
			FIDIV(FIMUL(viewlat, selecty - mapyorig), mapheight);
		setzoom(FIDIV(FIMUL(zoom, mapwidth), selectwidth));
		GrClearWindow(mapwid, GR_TRUE);
	}
}


static void
doposition(mp)
	GR_EVENT_MOUSE	*mp;
{
	GR_SIZE	temp;

	if (mp->wid != mapwid)
		return;

	if (coordenabled) {
		showcoords(GR_FALSE);
		ptrx = mp->x;
		ptry = mp->y;
		showcoords(GR_TRUE);
	}

	showselection(GR_FALSE);
	switch (selectmode) {
		case SELECT_SCALE:
			selectwidth = ABS(mp->x - selectx) * 2 + 1;
			selectheight = ABS(mp->y - selecty) * 2 + 1;
			temp = ((long) selectwidth) * selectyscale
				/ selectxscale;
			if (selectheight < temp)
				selectheight = temp;
			temp = ((long) selectheight) * selectxscale
				/ selectyscale;
			if (selectwidth < temp)
				selectwidth = temp;
			break;

		case SELECT_MOVE:
			selectx += (mp->x - selectptrx);
			selecty += (mp->y - selectptry);
			break;
	}

	selectptrx = mp->x;
	selectptry = mp->y;
	showselection(GR_TRUE);
}


static void
dokeydown(kp)
	GR_EVENT_KEYSTROKE	*kp;
{
	if (kp->wid != mapwid)
		return;

	if (selectmode != SELECT_NONE) {
		switch (kp->ch) {
			case 's':	/* scale selection */
				selectmode = SELECT_SCALE;
				break;

			case 'm':	/* move selection */
				selectmode = SELECT_MOVE;
				break;

			case '\033':	/* cancel selection */
			case 'c':
				showselection(GR_FALSE);
				selectmode = SELECT_NONE;
				break;
		}
		return;
	}

	switch (kp->ch) {
		case 'q':		/* quit */
		case 'Q':
			GrClose();
			exit(0);

		case 'r':		/* redraw total map */
		case ' ':
		case '\n':
			Longitude = ITOF(0);
			Latitude = ITOF(0);
			setzoom(ITOF(1));
			GrClearWindow(mapwid, GR_TRUE);
	}
}


/*
 * Draw or erase the current selection if any is defined.
 * The selection is a rectangle centered on a specified point, and with a
 * specified width and height.  Drawing and erasing the selection are the
 * same drawing operation because of the XOR operation.
 */
static void
showselection(show)
	GR_BOOL	show;		/* TRUE if show the selection */
{
	if ((show == 0) == (selectvisible == 0))
		return;
	if (selectmode == SELECT_NONE)
		return;
	GrRect(mapwid, xorgc, selectx - selectwidth / 2,
		selecty - selectheight / 2, selectwidth, selectheight);
	selectvisible = show;
}


/*
 * Draw or erase the coordinate string of the current pointer position.
 * Both of these are the same operation because of the XOR operation.
 */
static void
showcoords(show)
	GR_BOOL	show;		/* TRUE if show the coordinates */
{
	long	curlong;
	long	curlat;
	FLOAT	ptrlat;
	FLOAT	ptrlong;

	if (((show == 0) == (coordvisible == 0)) || !coordenabled)
		return;

	if (show) {
		ptrlat = FIDIV(FIMUL(viewlat, ptry), mapheight - 1);
		ptrlong = FIDIV(FIMUL(viewlong, ptrx), mapwidth - 1);

		curlat = FTOI(Latitude + latradius - ptrlat);
		curlong = FTOI(Longitude - longradius + ptrlong);

		if (curlong > 180*60)
			curlong -= 360*60;
		if (curlong < -180*60)
			curlong += 360*60;

		mintostr(coordstring, curlong);
		strcat(coordstring, "  ");
		mintostr(coordstring + strlen(coordstring), curlat);
	}

	GrText(mapwid, xorgc, coordx, coordy, coordstring, strlen(coordstring));
	coordvisible = show;
}


/*
 * Convert minutes to a string of the form "ddd'mm" and store it
 * into the indicated buffer.
 */
static void
mintostr(buf, min)
	char	*buf;
	long	min;
{
	if (min < 0) {
		min = -min;
		*buf++ = '-';
	}
	sprintf(buf, "%d'%02d", min / 60, min % 60);
}


#if 0
/*
 * Convert "ddd'mm" to mins
 */
static long
degtomin(s)
	char	*s;
{
	int	deg, min;
	char	str[10],*strchr(),*cp;

	strcpy(str,s);
	if (cp = strchr(str,'\047')) {
		*cp = '\0';
		min = atoi(++cp);
	} else
		min = 0;
	if ((deg = atoi(str)) < 0)
		min = -min;
	return(deg * 60 + min);
}
#endif


/*
 * Set the scale factors for the given zoom factor.
 * The factors 3 and 4 are here to compensate for the screen aspect ratio.
 */
static void
setzoom(newzoom)
	FLOAT	newzoom;
{
	zoom = newzoom;

	Y_Scale = FIDIV(FIMUL(zoom, mapheight * 3), 180 * 4);
	X_Scale = FIDIV(FIMUL(zoom, mapwidth), 360);

	viewlong = FFDIV(WSPAN, zoom);
	viewlat = FFDIV(HSPAN * 4 / 3, zoom);
	longradius = FIDIV(viewlong, 2);
	latradius = FIDIV(viewlat, 2);
}


/*
 * Read the database file and draw the world.
 */
static void
load(fn)
	char	*fn;
{
	register DBPOINT	*pp;
	DBPOINT		*pend;
	FLOAT		x, y, LonPrv, LatPrv;
	long		oldlong;
	GR_COORD	xnew, ynew;
	GR_COORD	xold, yold;
	GR_BOOL		is_out;
	GR_BOOL		was_out;
	GR_BOOL		newseg;
	GR_COLOR	oldcolor;
	GR_COLOR	newcolor;
	int		n;
	int		fh;
	DBPOINT		p[PCount];

	LonPrv = ITOF(0);
	LatPrv = ITOF(0);
	oldcolor = -1;
	is_out = GR_FALSE;
	was_out = GR_FALSE;

	fh = open(fn, O_BINARY);
	if (fh < 0) {
		GrClose();
		fprintf(stderr, "Cannot open %s\n", fn);
		exit(1);
	}

	while ((n = read(fh, p, PCount * POINTSize)) > 0) {
		for (pp = p,pend = p + n/POINTSize; pp < pend; pp++)
		{
			/* do displacement */
			x = ITOF(pp->Lon) - Longitude;
			y = ITOF(pp->Lat) - Latitude;

			/* wrap around for East-West */
			if (x < -HSPAN)
				x += WSPAN;
			if (x > HSPAN)
				x -= WSPAN;

			if (pp->Code > 5) {
				newcolor = code_colors[pp->Code / 1000];
				if (newcolor != oldcolor) {
					oldcolor = newcolor;
					GrSetGCForeground(mapgc, oldcolor);
				}
				newseg = GR_TRUE;
			}

			if (oldcolor == BLACK)
				goto go_on;

			/* ignore points outside magnified area */
			if ((x < -longradius || x > longradius ||
				y < -latradius || y > latradius))
			{
				is_out = 1;
				if (was_out) {		/* out to out */
					LonPrv = x;
					LatPrv = y;
					goto go_on;
				}

				/* in to out */
				xold = mapxorig + FTOI(FFMUL(LonPrv, X_Scale)) / 60;
				yold = mapyorig - FTOI(FFMUL(LatPrv, Y_Scale)) / 60;
			} else {			/* out to in */
				is_out = 0;
				if (was_out) {
					xold = mapxorig +
						FTOI(FFMUL(LonPrv, X_Scale)) / 60;
					yold = mapyorig -
						FTOI(FFMUL(LatPrv, Y_Scale)) / 60;
				}
				/* in to in */
			}
			LonPrv = x;
			LatPrv = y;

			/* scale points w/in area to interlace screen */
			xnew = mapxorig + FTOI(FFMUL(x, X_Scale)) / 60;
			ynew = mapyorig - FTOI(FFMUL(y, Y_Scale)) / 60;

			/* if new segment, move to place */
			if (newseg || ABS(oldlong - pp->Lon) > 180*60) {
				xold = xnew;
				yold = ynew;
			}
			oldlong = pp->Lon;

			GrLine(mapwid, mapgc, xold, yold, xnew, ynew);
			xold = xnew;
			yold = ynew;
go_on:
			was_out = is_out;
			newseg = GR_FALSE;
		}
	}
	close(fh);
}

/* END CODE */
