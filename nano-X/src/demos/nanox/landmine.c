/*
 * Landmine, the game.
 * Written for mini-X by David I. Bell.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#if UNIX
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#endif
#include "nano-X.h"

#if ELKS
#undef index
#endif

#define	MINSIZE		3		/* minimum size of board */
#define MAXSIZE		30		/* maximum size of board */
#define SIZE		15		/* default size of playing board */
#define	MINEPERCENT	15		/* default percentage of mines */
#define	SAVEFILE	"landmine.save"	/* default save file name */
#define MAGIC		649351261	/* magic number in save files */
#define	MAXPARAMS	200		/* maximum different game parameters */

#define FULLSIZE	(MAXSIZE + 2)	/* board size including borders */

#define	BOARDGAP	10		/* millimeter gap around board */
#define	RIGHTGAP	15		/* mm gap between board, right side */
#define	BUTTONGAP	20		/* mm gap between buttons */
#define	STATUSGAP	35		/* mm gap between buttons and status */

#define	BUTTONWIDTH	80		/* width of buttons (pixels) */
#define	BUTTONHEIGHT	25		/* height of buttons (pixels) */
#define	RIGHTSIDE	150		/* pixels to guarantee for right side */
#define	BOARDBORDER	2		/* border size around board */

/*
 * Print the number of steps taken.
 * This is used twice, and is a macro to guarantee that
 * the two printouts match.
 */
#define	PRINTSTEPS	printline(2, "Steps: %3d\n", steps)


/*
 * Typedefs local to this program.
 */
typedef	unsigned short	CELL;	/* cell value */
typedef	int		POS;	/* cell position */


/*
 * For defining bitmaps easily.
 */
#define	X	((unsigned) 1)
#define	_	((unsigned) 0)

#define	BITS(a,b,c,d,e,f,g,h,i) \
	(((((((((a*2+b)*2+c)*2+d)*2+e)*2+f)*2+g)*2+h)*2+i) << 7)


static	GR_BITMAP	twolegs_fg[] = {	/* two legs foreground */
	BITS(_,_,_,_,_,_,_,_,_),
	BITS(_,_,_,X,X,X,_,_,_),
	BITS(_,_,_,X,X,X,_,_,_),
	BITS(_,_,_,X,X,X,_,_,_),
	BITS(_,_,_,_,X,_,_,_,_),
	BITS(_,_,X,X,X,X,X,_,_),
	BITS(_,X,_,X,_,X,_,X,_),
	BITS(_,X,_,X,X,X,_,X,_),
	BITS(_,_,_,X,_,X,_,_,_),
	BITS(_,_,_,X,_,X,_,_,_),
	BITS(_,_,X,X,_,X,X,_,_),
	BITS(_,_,_,_,_,_,_,_,_)
};

static	GR_BITMAP	twolegs_bg[] = {	/* two legs background */
	BITS(_,_,X,X,X,X,X,_,_),
	BITS(_,_,X,X,X,X,X,_,_),
	BITS(_,_,X,X,X,X,X,_,_),
	BITS(_,_,X,X,X,X,X,_,_),
	BITS(_,X,X,X,X,X,X,X,_),
	BITS(X,X,X,X,X,X,X,X,X),
	BITS(X,X,X,X,_,X,X,X,X),
	BITS(X,X,X,X,X,X,X,X,X),
	BITS(X,X,X,X,X,X,X,X,X),
	BITS(_,X,X,X,X,X,X,X,_),
	BITS(_,X,X,X,X,X,X,X,_),
	BITS(_,X,X,X,X,X,X,X,_)
};


static	GR_BITMAP	oneleg_fg[] = {		/* one leg foreground */
	BITS(_,_,_,_,_,_,_,_,_),
	BITS(_,_,_,X,X,X,_,_,_),
	BITS(_,_,_,X,X,X,_,_,_),
	BITS(_,_,_,X,X,X,_,_,_),
	BITS(_,_,_,_,X,_,_,_,_),
	BITS(_,_,X,X,X,X,X,_,_),
	BITS(_,X,_,X,_,X,_,X,_),
	BITS(_,_,_,X,X,X,_,X,_),
	BITS(_,_,_,_,_,X,_,_,_),
	BITS(_,_,_,_,_,X,_,_,_),
	BITS(_,_,_,_,_,X,X,_,_),
	BITS(_,_,_,_,_,_,_,_,_),
};


static	GR_BITMAP	oneleg_bg[] = {		/* one leg background */
	BITS(_,_,X,X,X,X,X,_,_),
	BITS(_,_,X,X,X,X,X,_,_),
	BITS(_,_,X,X,X,X,X,_,_),
	BITS(_,_,X,X,X,X,X,_,_),
	BITS(_,X,X,X,X,X,X,X,_),
	BITS(X,X,X,X,X,X,X,X,X),
	BITS(X,X,X,X,_,X,X,X,X),
	BITS(X,X,X,X,X,X,X,X,X),
	BITS(_,_,X,X,X,X,X,X,X),
	BITS(_,_,_,_,X,X,X,X,_),
	BITS(_,_,_,_,X,X,X,X,_),
	BITS(_,_,_,_,X,X,X,X,_)
};


static	GR_BITMAP	noleg_fg[] = {		/* no legs foreground */
	BITS(_,_,_,_,_,_,_,_,_),
	BITS(_,_,_,X,X,X,_,_,_),
	BITS(_,_,_,X,X,X,_,_,_),
	BITS(_,_,_,X,X,X,_,_,_),
	BITS(_,_,_,_,X,_,_,_,_),
	BITS(_,_,X,X,X,X,X,_,_),
	BITS(_,X,_,X,_,X,_,X,_),
	BITS(_,_,_,X,X,X,_,_,_),
	BITS(_,_,_,_,_,_,_,_,_),
	BITS(_,_,_,_,_,_,_,_,_),
	BITS(_,_,_,_,_,_,_,_,_),
	BITS(_,_,_,_,_,_,_,_,_),
};


static	GR_BITMAP	noleg_bg[] = {		/* no legs background */
	BITS(_,_,X,X,X,X,X,_,_),
	BITS(_,_,X,X,X,X,X,_,_),
	BITS(_,_,X,X,X,X,X,_,_),
	BITS(_,_,X,X,X,X,X,_,_),
	BITS(_,X,X,X,X,X,X,X,_),
	BITS(X,X,X,X,X,X,X,X,X),
	BITS(X,X,X,X,_,X,X,X,X),
	BITS(X,X,X,X,X,X,X,X,X),
	BITS(_,_,X,X,X,X,X,_,_),
	BITS(_,_,_,_,_,_,_,_,_),
	BITS(_,_,_,_,_,_,_,_,_),
	BITS(_,_,_,_,_,_,_,_,_)
};


/*
 * Components of a cell.
 */
#define F_EMPTY		' '		/* default value for empty square */
#define F_REMEMBER	'*'		/* character to remember mine */
#define F_WRONG		'X'		/* character to remember wrong guess */
#define F_DISPLAY	0xff		/* character to be displayed here */
#define F_MINE		0x100		/* TRUE if a mine is here */
#define F_EDGE		0x200		/* TRUE if this is edge of the world */
#define F_OLD		0x400		/* TRUE if been at this square before */
#define F_REACH		0x800		/* TRUE if can reach this square */
#define F_FLAGS		0xff00		/* all flags */


/*
 * The status of the game.
 * This structure is read and written from the save file.
 */
static	struct status {			/* status of games */
	long	s_magic;		/* magic number */
	short	s_playing;		/* TRUE if playing a game */
	short	s_size;			/* current size of board */
	short	s_mines;		/* current number of mines on board */
	short	s_legs;			/* number of legs left */
	short	s_steps;		/* number of steps taken this game */
	short	s_index;		/* current game parameter index */
	short	s_sizeparam[MAXPARAMS];	/* table of size parameters */
	short	s_mineparam[MAXPARAMS];	/* table of mine parameters */
	long	s_games0[MAXPARAMS];	/* games finished with no legs */
	long	s_games1[MAXPARAMS];	/* games finished with one leg */
	long	s_games2[MAXPARAMS];	/* games finished with two legs */
	long	s_steps0[MAXPARAMS];	/* steps taken in no leg games */
	long	s_steps1[MAXPARAMS];	/* steps taken in one leg games */
	long	s_steps2[MAXPARAMS];	/* steps taken in two leg games */
	CELL	s_board[FULLSIZE*FULLSIZE];	/* board layout */
} st;


/*
 * Definitions to make structure references easy.
 */
#define magic		st.s_magic
#define	playing		st.s_playing
#define size		st.s_size
#define mines		st.s_mines
#define legs		st.s_legs
#define	steps		st.s_steps
#define	index		st.s_index
#define	sizeparam	st.s_sizeparam
#define	mineparam	st.s_mineparam
#define games0		st.s_games0
#define games1		st.s_games1
#define games2		st.s_games2
#define	steps0		st.s_steps0
#define	steps1		st.s_steps1
#define	steps2		st.s_steps2
#define board		st.s_board


#define boardpos(row, col)	(((row) * FULLSIZE) + (col))
#define ismine(cell)		((cell) & F_MINE)
#define isedge(cell)		((cell) & F_EDGE)
#define isold(cell)		((cell) & F_OLD)
#define isseen(cell)		(((cell) & F_DISPLAY) == F_REMEMBER)
#define isknown(cell)		(((cell) & F_DISPLAY) != F_EMPTY)
#define displaychar(cell)	((cell) & F_DISPLAY)
#define badsquare(n)		(((n) <= 0) || ((n) > size))


/*
 * Offsets for accessing adjacent cells.
 */
static	POS	steptable[8] = {
	FULLSIZE, -FULLSIZE, 1, -1, FULLSIZE-1,
	FULLSIZE+1, -FULLSIZE-1, -FULLSIZE+1
};


static	GR_WINDOW_ID	mainwid;	/* main window id */
static	GR_WINDOW_ID	boardwid;	/* board window id */
static	GR_WINDOW_ID	statwid;	/* status display window id */
static	GR_WINDOW_ID	quitwid;	/* window id for quit button */
static	GR_WINDOW_ID	savewid;	/* window id for save button */
static	GR_WINDOW_ID	newgamewid;	/* window id for new game button */

static	GR_GC_ID	boardgc;	/* graphics context for board */
static	GR_GC_ID	cleargc;	/* GC for clearing cell of board */
static	GR_GC_ID	redgc;		/* GC for drawing red */
static	GR_GC_ID	greengc;	/* GC for drawing green */
static	GR_GC_ID	blackgc;	/* GC for drawing black */
static	GR_GC_ID	delaygc;	/* GC for delaying */
static	GR_GC_ID	statgc;		/* GC for status window */
static	GR_GC_ID	buttongc;	/* GC for drawing buttons */
static	GR_GC_ID	xorgc;		/* GC for inverting things */

static	GR_SIZE		xp;		/* pixels for x direction per square */
static	GR_SIZE		yp;		/* pixels for y direction per square */
static	GR_SIZE		statwidth;	/* width of window drawing text in */
static	GR_SIZE		statheight;	/* height of window drawing text in */
static	GR_SIZE		charheight;	/* height of characters */
static	GR_COORD	charxpos;	/* current X position for characters */
static	GR_COORD	charypos;	/* current Y position for characters */

static	GR_SCREEN_INFO	si;		/* window information */
static	GR_FONT_INFO	fi;		/* font information */

static	char	*savefile;		/* filename for saving game */


/*
 * Procedures.
 */
static	void	printline(GR_COORD, char *, ...);
static	void	newline();
static	void	delay();
static	void	dokey();
static	void	readevent();
static	void	doexposure();
static	void	dobutton();
static	void	drawbomb();
static	void	drawstatus();
static	void	drawbutton();
static	void	drawboard();
static	void	drawcell();
static	void	cellcenter();
static	void	clearcell();
static	void	newgame();
static	void	moveto();
static	void	setcursor();
static	void	togglecell();
static	void	gameover();
static	void	readgame();
static	void	findindex();
static	POS	findcell();
static	GR_BOOL	checkpath();
static	GR_BOOL	writegame();

int
main(argc,argv)
	int argc;
	char **argv;
{
	GR_COORD	x;
	GR_COORD	y;
	GR_SIZE		width;
	GR_SIZE		height;
	GR_COORD	rightx;		/* x coordinate for right half stuff */
	GR_BOOL		setsize;	/* TRUE if size of board is set */
	GR_BOOL		setmines;	/* TRUE if number of mines is set */
	GR_SIZE		newsize = 0;	/* desired size of board */
	GR_COUNT	newmines = 0;	/* desired number of mines */

	setmines = GR_FALSE;
	setsize = GR_FALSE;

	argc--;
	argv++;
	while ((argc > 0) && (**argv == '-')) {
		switch (argv[0][1]) {
			case 'm':
				if (argc <= 0) {
					fprintf(stderr, "Missing mine count\n");
					exit(1);
				}
				argc--;
				argv++;
				newmines = atoi(*argv);
				setmines = GR_TRUE;
				break;

			case 's':
				if (argc <= 0) {
					fprintf(stderr, "Missing size\n");
					exit(1);
				}
				argc--;
				argv++;
				newsize = atoi(*argv);
				setsize = GR_TRUE;
				break;

			default:
				fprintf(stderr, "Unknown option \"-%c\"\n",
					argv[0][1]);
				exit(1);
		}
		argc--;
		argv++;
	}
	if (argc > 0)
		savefile = *argv;

	srand(time(0));

	readgame(savefile);

	if (setsize) {
		if ((newsize < MINSIZE) || (newsize > MAXSIZE)) {
			fprintf(stderr, "Illegal board size\n");
			exit(1);
		}
		if (newsize != size) {
			if (steps && playing) {
				fprintf(stderr,
					"Cannot change size while game is in progress\n");
				exit(1);
			}
			playing = GR_FALSE;
			size = newsize;
			if (!playing)
				mines = (size * size * MINEPERCENT) / 100;
		}
	}

	if (setmines) {
		if ((newmines <= 0) || ((newmines > (size * size) / 2))) {
			fprintf(stderr, "Illegal number of mines\n");
			exit(1);
		}
		if (newmines != mines) {
			if (steps && playing) {
				fprintf(stderr,
					"Cannot change mines while game is in progress\n");
				exit(1);
			}
			playing = GR_FALSE;
			mines = newmines;
		}
	}

	findindex();

	/*
	 * Parameters of the game have been verified.
	 * Now open the graphics and play the game.
	 */
	if (GrOpen() < 0) {
		fprintf(stderr, "Cannot open graphics\n");
		exit(1);
	}

	GrGetScreenInfo(&si);
	GrGetFontInfo(0, &fi);
	charheight = fi.height;

	/*
	 * Create the main window which will contain all the others.
	 */
	mainwid = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, si.cols, si.rows,
		0, BLACK, WHITE);

	/*
	 * Create the board window which lies at the left side.
	 * Make the board square, and as large as possible while still
	 * leaving room to the right side for statistics and buttons.
	 */
	width = si.cols - RIGHTSIDE - (si.xdpcm * RIGHTGAP / 10) - BOARDBORDER * 2;
	height = (((long) width) * si.ydpcm) / si.xdpcm;
	if (height > si.rows /* - y * 2*/) {
		height = si.rows - BOARDBORDER * 2;
		width = (((long) height) * si.xdpcm) / si.ydpcm;
	}
	xp = width / size;
	yp = height / size;
	width = xp * size - 1;
	height = yp * size - 1;
	x = BOARDBORDER;
	y = (si.rows - height) / 2;
	rightx = x + width + (si.xdpcm * RIGHTGAP / 10);
	boardwid = GrNewWindow(mainwid, x, y, width, height, BOARDBORDER,
		BLUE, WHITE);

	/*
	 * Create the buttons.
	 */
	x = rightx;
	y = (si.ydpcm * BOARDGAP / 10);
	quitwid = GrNewWindow(mainwid, x, y, BUTTONWIDTH, BUTTONHEIGHT,
		1, RED, WHITE);

	y += (si.ydpcm * BUTTONGAP / 10);
	savewid = GrNewWindow(mainwid, x, y, BUTTONWIDTH, BUTTONHEIGHT,
		1, GREEN, WHITE);

	y += (si.ydpcm * BUTTONGAP / 10);
	newgamewid = GrNewWindow(mainwid, x, y, BUTTONWIDTH, BUTTONHEIGHT,
		1, GREEN, WHITE);

	/*
	 * Create the statistics window.
	 */
	x = rightx;
	y += (si.ydpcm * STATUSGAP / 10);
	width = si.cols - x;
	height = si.rows - y;
	statwid = GrNewWindow(mainwid, x, y, width, height, 0,
		0, 0);
	statwidth = width;
	statheight = height;

	/*
	 * Create the GC for drawing the board.
	 */
	boardgc = GrNewGC();
	cleargc = GrNewGC();
	delaygc = GrNewGC();
	redgc = GrNewGC();
	greengc = GrNewGC();
	statgc = GrNewGC();
	blackgc = GrNewGC();
	buttongc = GrNewGC();
	xorgc = GrNewGC();
	GrSetGCBackground(boardgc, BLUE);
	GrSetGCForeground(cleargc, BLUE);
	GrSetGCForeground(redgc, RED);
	GrSetGCForeground(greengc, GREEN);
	GrSetGCForeground(statgc, GRAY);
	GrSetGCForeground(delaygc, BLACK);
	GrSetGCForeground(blackgc, BLACK);
	GrSetGCMode(delaygc, GR_MODE_XOR);
	GrSetGCMode(xorgc, GR_MODE_XOR);
	GrSetGCUseBackground(boardgc, GR_FALSE);
	GrSetGCUseBackground(buttongc, GR_FALSE);

	GrSelectEvents(boardwid, GR_EVENT_MASK_EXPOSURE |
		GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_KEY_DOWN);

	GrSelectEvents(statwid, GR_EVENT_MASK_EXPOSURE);

	GrSelectEvents(quitwid, GR_EVENT_MASK_EXPOSURE |
		GR_EVENT_MASK_BUTTON_DOWN);

	GrSelectEvents(newgamewid, GR_EVENT_MASK_EXPOSURE |
		GR_EVENT_MASK_BUTTON_DOWN);

	GrSelectEvents(savewid, GR_EVENT_MASK_EXPOSURE |
		GR_EVENT_MASK_BUTTON_DOWN);

	setcursor();

	GrMapWindow(mainwid);
	GrMapWindow(boardwid);
	GrMapWindow(statwid);
	GrMapWindow(quitwid);
	GrMapWindow(savewid);
	GrMapWindow(newgamewid);

	if (!playing)
		newgame();

	while (GR_TRUE)
		readevent();
}


/*
 * Read the next event and handle it.
 */
static void
readevent()
{
	GR_EVENT	event;

	GrGetNextEvent(&event);

	switch (event.type) {
		case GR_EVENT_TYPE_BUTTON_DOWN:
			dobutton(&event.button);
			break;

		case GR_EVENT_TYPE_EXPOSURE:
			doexposure(&event.exposure);
			break;

		case GR_EVENT_TYPE_KEY_DOWN:
			dokey(&event.keystroke);
			break;
	}
}


/*
 * Handle exposure events.
 */
static void
doexposure(ep)
	GR_EVENT_EXPOSURE	*ep;
{
	if (ep->wid == boardwid) {
		drawboard();
		return;
	}

	if (ep->wid == statwid) {
		drawstatus();
		return;
	}

	if (ep->wid == quitwid) {
		drawbutton(quitwid, "QUIT");
		return;
	}

	if (ep->wid == savewid) {
		drawbutton(savewid, "SAVE GAME");
		return;
	}

	if (ep->wid == newgamewid) {
		drawbutton(newgamewid, "NEW GAME");
		return;
	}
}


/*
 * Here when we get a button down event.
 */
static void
dobutton(bp)
	GR_EVENT_BUTTON		*bp;
{
	if (bp->wid == boardwid) {
		moveto(findcell(bp->x, bp->y));
		return;
	}

	if (bp->wid == quitwid) {
		GrFillRect(quitwid, xorgc, 0, 0, BUTTONWIDTH, BUTTONHEIGHT);
		GrFlush();
		if (savefile)
			writegame(savefile);
		GrClose();
		exit(0);
	}

	if (bp->wid == savewid) {
		GrFillRect(savewid, xorgc, 0, 0, BUTTONWIDTH, BUTTONHEIGHT);
		GrFlush();
		if (savefile == NULL)
			savefile = SAVEFILE;
		if (writegame(savefile))
			write(1, "\007", 1);
		else
			delay();
		GrFillRect(savewid, xorgc, 0, 0, BUTTONWIDTH, BUTTONHEIGHT);
	}

	if (bp->wid == newgamewid) {
		GrFillRect(newgamewid, xorgc, 0, 0, BUTTONWIDTH, BUTTONHEIGHT);
		GrFlush();
		/*if (playing)
			write(1, "\007", 1);
		else {*/
			newgame();
			delay();
		/*}*/
		GrFillRect(newgamewid, xorgc, 0, 0, BUTTONWIDTH, BUTTONHEIGHT);
	}
}


/*
 * Here when we get a keypress in a window.
 */
static void
dokey(kp)
	GR_EVENT_KEYSTROKE	*kp;
{
	if ((kp->wid != boardwid) || !playing)
		return;

	switch (kp->ch) {
		case ' ':			/* remember or forget mine */
			togglecell(findcell(kp->x, kp->y));
			break;
	}
}


/*
 * Redraw the board.
 */
static void
drawboard()
{
	GR_COORD	row;
	GR_COORD	col;

	for (row = 1; row < size; row++) {
		GrLine(boardwid, boardgc, 0, row * yp - 1, size * xp - 1,
			row * yp - 1);
		GrLine(boardwid, boardgc, row * xp - 1, 0,
			row * xp - 1, size * yp - 1);
	}
	for (row = 0; row < FULLSIZE; row++) {
		for (col = 0; col < FULLSIZE; col++) {
			drawcell(boardpos(row, col));
		}
	}
}


/*
 * Draw a cell on the board.
 */
static void
drawcell(pos)
	POS		pos;		/* position to be drawn */
{
	GR_COORD	x;
	GR_COORD	y;
	GR_SIZE		chwidth;
	GR_SIZE		chheight;
	GR_SIZE		chbase;
	CELL		cell;
	GR_CHAR		ch;

	cell = board[pos];
	if (!isknown(cell))
		return;

	ch = displaychar(cell);
	if (ch == F_WRONG) {
		drawbomb(pos, greengc, GR_FALSE);
		return;
	}

	if (isold(cell)) {
		clearcell(pos);
		cellcenter(pos, &x, &y);
		GrGetGCTextSize(boardgc, &ch, 1, &chwidth, &chheight,
			&chbase);
		GrText(boardwid, boardgc, x - chwidth / 2 + 1,
			y + chheight / 2, &ch, 1);
		return;
	}

	drawbomb(pos, redgc, GR_FALSE);
}


/*
 * Clear a particular cell.
 */
static void
clearcell(pos)
	POS		pos;		/* position to be cleared */
{
	GR_COORD	row;
	GR_COORD	col;

	row = pos / FULLSIZE;
	col = pos % FULLSIZE;
	GrFillRect(boardwid, cleargc, col * xp - xp, row * yp - yp,
		xp - 1, yp - 1);
}


/*
 * Draw a bomb in a window using the specified GC.
 * The bomb is animated and the terminal is beeped if necessary.
 */
static void
drawbomb(pos, gc, animate)
	POS		pos;		/* position to draw bomb at */
	GR_GC_ID	gc;		/* GC for drawing (red or green) */
	GR_BOOL		animate;	/* TRUE to animate the bomb */
{
	GR_COORD	x;
	GR_COORD	y;
	GR_COUNT	count;

	if (animate)
		write(1, "\007", 1);

	cellcenter(pos, &x, &y);

	count = (animate ? 8 : 1);
	for (;;) {
		GrFillEllipse(boardwid, gc, x, y, xp / 2 - 3, yp / 2 - 3);
		if (--count == 0)
			return;
		delay();
		clearcell(pos);
		delay();
	}
}


/*
 * Draw a button which has a specified label string centered in it.
 */
static void
drawbutton(window, label)
	GR_WINDOW_ID	window;
	char		*label;
{
	GR_SIZE		width;
	GR_SIZE		height;
	GR_SIZE		base;

	GrGetGCTextSize(buttongc, label, strlen(label), &width, &height, &base);
	GrText(window, buttongc, (BUTTONWIDTH - width) / 2,
		(BUTTONHEIGHT - height) / 2 + height - 1,
		label, strlen(label));
}


/*
 * Set the cursor as appropriate.
 * The cursor changes depending on the number of legs left.
 */
static void
setcursor()
{
	GR_BITMAP	*fgbits;	/* bitmap for foreground */
	GR_BITMAP	*bgbits;	/* bitmap for background */

	switch (legs) {
		case 0:
			fgbits = noleg_fg;
			bgbits = noleg_bg;
			break;
		case 1:
			fgbits = oneleg_fg;
			bgbits = oneleg_bg;
			break;
		default:
			fgbits = twolegs_fg;
			bgbits = twolegs_bg;
			break;
	}
	GrSetCursor(boardwid, 9, 12, 4, 6, WHITE, BLACK, fgbits, bgbits);
}


/*
 * Delay for a while so that something can be seen.
 * This is done by drawing a large rectangle over the window using a mode
 * of XOR with the value of 0, (which does nothing except waste time).
 */
static void
delay()
{
	GR_COUNT	i;

	for (i = 0; i < 1; i++) {
		GrFillRect(boardwid, delaygc, 0, 0, xp * size - 1,
			yp * size - 1);
		GrFlush();
	}
}


/*
 * Calculate the coordinates of the center of a cell on the board.
 * The coordinates are relative to the origin of the board window.
 */
static void
cellcenter(pos, retx, rety)
	POS		pos;		/* position to find center of */
	GR_COORD	*retx;		/* returned X coordinate */
	GR_COORD	*rety;		/* returned Y coordinate */
{
	*retx = (pos % FULLSIZE) * xp - 1 - xp / 2;
	*rety = (pos / FULLSIZE) * yp - 1 - yp / 2;
}


/*
 * Draw the status information in the status window.
 */
static void
drawstatus()
{
	long	score;
	long	allsteps;
	long	games;

	score = 0;
	games = games0[index];
	allsteps = steps0[index];
	score += games1[index];
	games += games1[index];
	allsteps += steps1[index];
	score += games2[index] * 2;
	games += games2[index];
	allsteps += steps2[index];

	printline(0, "Size:   %2d\n", size);
	printline(1, "Mines: %3d\n", mines);
	PRINTSTEPS;
	printline(3, "Legs:    %d\n", legs);

	printline(5, "Won games:  %3d\n", games2[index]);
	printline(6, "1-leg games:%3d\n", games1[index]);
	printline(7, "Lost games: %3d\n", games0[index]);

	if (games) {
		printline(9, "Legs/game: %3d.%03d\n", score / games,
			((score * 1000) / games) % 1000);

		printline(10, "Steps/game:%3d.%03d\n", allsteps / games,
			((allsteps * 1000) / games) % 1000);
	}
}


/*
 * Printf routine for windows, which can print at particular lines.
 * A negative line number means continue printing at the previous location.
 * Assumes the status window for output.
 */
static void printline(GR_COORD row, char * fmt, ...)
{
	va_list		ap;
	GR_COUNT	cc;
	GR_SIZE		width;
	char		*cp;
	char		buf[256];

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);

	if (row >= 0) {
		charxpos = 0;
		charypos = charheight * row + charheight - 1;
	}

	cp = buf;
	for (;;) {
		cc = 0;
		width = 0;
		while (*cp >= ' ') {
			width += fi.widths[(int)*cp++];
			cc++;
		}
		if (width) {
			GrText(statwid, statgc, charxpos, charypos,
				cp - cc, cc);
			charxpos += width;
		}

		switch (*cp++) {
			case '\0':
				return;
			case '\n':
				newline();
				break;
			case '\r':
				charxpos = 0;
				break;
		}
	}
}


/*
 * Clear the remainder of the line and move to the next line.
 * This assumes output is in the status window.
 */
static void
newline()
{
	GrFillRect(statwid, blackgc, charxpos, charypos - charheight + 1,
		statwidth - charxpos, charheight);
	charxpos = 0;
	charypos += charheight;
}


/*
 * Translate a board window coordinate into a cell position.
 * If the coordinate is outside of the window, or exactly on one
 * of the interior lines, then a coordinate of 0 is returned.
 */
static POS
findcell(x, y)
	GR_COORD	x;
	GR_COORD	y;
{
	GR_COORD	row;
	GR_COORD	col;

	if (((x % xp) == 0) || ((y % yp) == 0))
		return 0;
	row = (y / yp) + 1;
	col = (x / xp) + 1;
	if ((row <= 0) || (row > size) || (col <= 0) || (col > size))
		return 0;
	return boardpos(row, col);
}


/*
 * Initialize the board for playing
 */
static void
newgame()
{
	GR_COORD	row;
	GR_COORD	col;
	GR_COUNT	count;
	CELL		cell;
	POS		pos;

	for (row = 0; row < FULLSIZE; row++) {
		for (col = 0; col < FULLSIZE; col++) {
			cell = F_EMPTY;
			if (badsquare(row) || badsquare(col))
				cell |= F_EDGE;
			board[boardpos(row, col)] = cell;
		}
	}

	playing = GR_TRUE;
	count = 0;
	legs = 2;
	steps = 0;
	drawstatus();
	setcursor();

	while (count < mines) {
		do {
			row = (rand() / 16) % (size * size + 1);
		} while (row == (size * size));

		col = (row % size) + 1;
		row = (row / size) + 1;
		pos = boardpos(row, col);

		if ((pos == boardpos(1,1)) || (pos == boardpos(1,2)) ||
			(pos == boardpos(2,1)) || (pos == boardpos(2,2)) ||
			(pos == boardpos(size,size)))
				continue;

		if (!ismine(board[pos]) && checkpath(pos))
			count++;
	}

	board[boardpos(1,1)] = (F_OLD | '0');

	GrClearWindow(boardwid, GR_TRUE);
}


/*
 * Check to see if there is still a path from the top left corner to the
 * bottom right corner, if a new mine is placed at the indicated position.
 * Returns GR_TRUE if mine was successfully placed.
 */
static GR_BOOL
checkpath(pos)
	POS		pos;		/* position to place mine at */
{
	CELL		*bp;		/* current board position */
	CELL		*endbp;		/* ending position */
	POS		endpos;		/* ending position */
	GR_COUNT	count;		/* number of neighbors */
	GR_COUNT	i;		/* loop counter */
	GR_BOOL		more;		/* GR_TRUE if new square reached */

	/*
	 * Begin by assuming there is a mine at the specified location,
	 * and then count neighbors.  If there are less than two other
	 * mines or edge squares, then there must still be a path.
	 */
	board[pos] |= F_MINE;

	count = 0;

	for (i = 7; i >= 0; i--) {
		if (board[pos + steptable[i]] & (F_MINE | F_EDGE))
		count++;
	}

	if (count < 2)
		return GR_TRUE;

	/*
	 * Two or more neighbors, so we must do the full check.
	 * First clear the reach flag, except for the top left corner.
	 */
	endpos = boardpos(size, size);
	bp = &board[endpos];
	endbp = bp;
	while (bp != board)
		*bp-- &= ~F_REACH;
	board[boardpos(1,1)] |= F_REACH;

	/*
	 * Now loop looking for new squares next to already reached squares.
	 * Stop when no more changes are found, or when the lower right
	 * corner is reached.
	 */
	do {
		more = GR_FALSE;
		for (bp = &board[boardpos(1,1)]; bp != endbp; bp++) {
			if (*bp & F_REACH) {
				for (i = 7; i >= 0; i--) {
					if ((bp[steptable[i]] & (F_MINE | F_REACH | F_EDGE)) == 0) {
						bp[steptable[i]] |= F_REACH;
						more = GR_TRUE;
					}
				}
			}
		}

		if (board[endpos] & F_REACH)
			return GR_TRUE;
	} while (more);

	/*
	 * Cannot reach the lower right corner, so remove the mine and fail.
	 */
	board[pos] &= ~F_MINE;

	return GR_FALSE;
}


/*
 * Move to a particular position and see if we hit a mine.
 * If not, then count the number of mines adjacent to us so it can be seen.
 * If we are stepping onto a location where we remembered a mine is at,
 * then don't do it.  Moving is only allowed to old locations, or to
 * locations adjacent to old ones.
 */
static void
moveto(newpos)
	POS		newpos;		/* position to move to */
{
	POS		fixpos;		/* position to fix up */
	CELL		cell;		/* current cell */
	GR_COUNT	count;		/* count of cells */
	GR_COUNT	i;		/* index for neighbors */

	if ((newpos < 0) || (newpos >= (FULLSIZE * FULLSIZE)) || !playing)
		return;

	cell = board[newpos];

	if (isedge(cell) || (isseen(cell)) || isold(cell))
		return;

	count = isold(cell);
	for (i = 0; i < 8; i++)
		if (isold(board[newpos + steptable[i]]))
			count++;

	if (count <= 0)
		return;

	cell = (cell & F_FLAGS) | F_OLD;
	steps++;

	PRINTSTEPS;

	if (ismine(cell)) {		/* we hit a mine */
		legs--;
		board[newpos] = (F_REMEMBER | F_MINE);
		cell = (F_EMPTY | F_OLD);
		board[newpos] = cell;
		drawbomb(newpos, redgc, GR_TRUE);
		clearcell(newpos);
		setcursor();
		for (i = 0; i < 8; i++) {
			fixpos = newpos + steptable[i];
			if (isold(board[fixpos])) {
				board[fixpos]--;
				drawcell(fixpos);
			}
		}
		drawstatus();
	}

	count = 0;
	for (i = 0; i < 8; i++)
		if (ismine(board[newpos + steptable[i]]))
			count++;
	board[newpos] = cell | (count + '0');

	drawcell(newpos);

	if ((legs <= 0) || (newpos == boardpos(size,size)))
		gameover();
}


/*
 * Remember or forget the location of a mine.
 * This is for informational purposes only and does not affect anything.
 */
static void
togglecell(pos)
	POS	pos;		/* position to toggle */
{
	CELL	cell;

	if ((pos <= 0) || !playing)
		return;

	cell = board[pos];
	if (isknown(cell)) {
		if (!isseen(cell))
			return;
		board[pos] = (board[pos] & F_FLAGS) | F_EMPTY;
		clearcell(pos);
		return;
	}

	board[pos] = (board[pos] & F_FLAGS) | F_REMEMBER;
	drawcell(pos);
}


/*
 * Here when the game is over.
 * Show where the mines are, and give the results.
 */
static void
gameover()
{
	POS	pos;
	CELL	cell;

	playing = GR_FALSE;
	switch (legs) {
		case 0:
			games0[index]++;
			steps0[index] += steps;
			break;
		case 1:
			games1[index]++;
			steps1[index] += steps;
			break;
		case 2:
			games2[index]++;
			steps2[index] += steps;
			break;
	}

	for (pos = 0; pos < (FULLSIZE * FULLSIZE); pos++) {
		cell = board[pos];
		if (isseen(cell))
			cell = (cell & F_FLAGS) | F_WRONG;
		if (ismine(cell))
			cell = (cell & F_FLAGS) | F_REMEMBER;
		board[pos] = cell;
	}

	drawboard();
	drawstatus();
}


/*
 * Search the game parameter table for the current board size and
 * number of mines, and set the index for those parameters so that
 * the statistics can be accessed.  Allocates a new index if necessary.
 */
static void
findindex()
{
	for (index = 0; index < MAXPARAMS; index++) {
		if ((sizeparam[index] == size) && (mineparam[index] == mines))
			return;
	}
	for (index = 0; index < MAXPARAMS; index++) {
		if (sizeparam[index] == 0) {
			sizeparam[index] = size;
			mineparam[index] = mines;
			return;
		}
	}
	fprintf(stderr, "Too many parameters in save file\n");
	exit(1);
}


/*
 * Read in a saved game if available, otherwise start from scratch.
 * Exits if an error is encountered.
 */
static void
readgame(name)
	char	*name;		/* filename */
{
	int	fd;

	fd = -1;
	if (name)
		fd = open(name, 0);

	if (fd < 0) {
		magic = MAGIC;
		size = SIZE;
		mines = (size * size * MINEPERCENT) / 100;
		playing = GR_FALSE;
		return;
	}

	if (read(fd, (char *)&st, sizeof(st)) != sizeof(st))
		magic = 0;
	close(fd);

	if ((magic != MAGIC) || (size > MAXSIZE)) {
		fprintf(stderr, "Save file format is incorrect\n");
		exit(1);
	}
}


/*
 * Write the current game to a file.
 * Returns nonzero on an error.
 */
static GR_BOOL
writegame(name)
	char	*name;		/* filename */
{
	int	fd;

	if (name == NULL)
		return GR_TRUE;

	fd = creat(name, 0666);
	if (fd < 0)
		return GR_TRUE;

	if (write(fd, (char *)&st, sizeof(st)) != sizeof(st)) {
		close(fd);
		return GR_TRUE;
	}
	close(fd);
	return GR_FALSE;
}

/* END CODE */
