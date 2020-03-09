#ifndef	_SERV_H
#define	_SERV_H
/*
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Private definitions for the graphics server.
 * These definitions are not to be used by the clients.
 * Clients must call the GrXXXX routines instead of these GsXXXX routines.
 */
#include "nano-X.h"

/*
 * Drawing types.
 */
typedef	int	GR_DRAW_TYPE;

#define	GR_DRAW_TYPE_NONE	((GR_DRAW_TYPE) 0)	/* none or error */
#define	GR_DRAW_TYPE_WINDOW	((GR_DRAW_TYPE) 1)	/* windows */
#define	GR_DRAW_TYPE_PIXMAP	((GR_DRAW_TYPE) 2)	/* pixmaps */

/*
 * List of elements for events.
 */
typedef	struct gr_event_list GR_EVENT_LIST;
struct gr_event_list {
	GR_EVENT_LIST	*next;		/* next element in list */
	GR_EVENT	event;		/* event */
};

/*
 * Data structure to keep track of state of clients.
 */
typedef struct gr_client GR_CLIENT;
struct gr_client {
	int		id;		/* client id and socket descriptor */
	GR_EVENT_LIST	*eventhead;	/* head of event chain (or NULL) */
	GR_EVENT_LIST	*eventtail;	/* tail of event chain (or NULL) */
	GR_EVENT_ERROR	errorevent;	/* waiting error event to deliver */
	GR_CLIENT	*next;		/* the next client in the list */
	GR_CLIENT	*prev;		/* the previous client in the list */
	int		waiting_for_event; /* used to implement GrGetNextEvent*/
};

/*
 * Structure to remember clients associated with events.
 */
typedef	struct gr_event_client	GR_EVENT_CLIENT;
struct gr_event_client	{
	GR_EVENT_CLIENT	*next;		/* next interested client or NULL */
	GR_EVENT_MASK	eventmask;	/* events client wants to see */
	GR_CLIENT	*client;	/* client who is interested */
};

/*
 * Structure to remember graphics contexts.
 */
typedef	struct gr_gc	GR_GC;
struct gr_gc {
	GR_GC_ID	id;		/* graphics context id */
	GR_MODE		mode;		/* drawing mode */
	GR_FONT		font;		/* current font number */
	GR_COLOR	foreground;	/* foreground color */
	GR_COLOR	background;	/* background color */
	GR_BOOL		usebackground;	/* actually display the background */
	GR_BOOL		changed;	/* graphics context has been changed */
	GR_GC		*next;		/* next graphics context */
	int		usecount;	/* usecount */
};

/*
 * Structure to remember cursor definitions.
 */
typedef	struct {
	int		usecount;	/* use counter */
	SWCURSOR	cursor;
} GR_CURSOR;


/*
 * Window structure
 */
typedef struct gr_window GR_WINDOW;
struct gr_window {
	GR_WINDOW_ID	id;		/* window id */
	GR_WINDOW	*parent;	/* parent window */
	GR_WINDOW	*children;	/* first child window */
	GR_WINDOW	*siblings;	/* next sibling window */
	GR_WINDOW	*next;		/* next window in complete list */
	GR_COORD	x;		/* absolute x position */
	GR_COORD	y;		/* absolute y position */
	GR_SIZE		width;		/* width */
	GR_SIZE		height;		/* height */
	GR_SIZE		bordersize;	/* size of border */
	GR_COLOR	bordercolor;	/* color of border */
	GR_COLOR	background;	/* background color */
	GR_EVENT_MASK	nopropmask;	/* events not to be propagated */
	GR_EVENT_CLIENT	*eventclients;	/* clients interested in events */
	GR_CURSOR	*cursor;	/* cursor for this window */
	GR_BOOL		mapped;		/* TRUE if explicitly mapped */
	GR_COUNT	unmapcount;	/* count of reasons not really mapped */
	GR_BOOL		output;		/* TRUE if window can do output */
};


/*
 * Pixmap structure (not yet implemented).
 * Pixmaps of 2 colors use bitmaps, otherwise they use color values.
 */
typedef struct gr_pixmap	GR_PIXMAP;
struct gr_pixmap {
	GR_PIXMAP_ID	id;		/* pixmap id */
	GR_PIXMAP	*next;		/* next pixmap in list */
	GR_SIZE		width;		/* width of pixmap */
	GR_SIZE		height;		/* height of pixmap */
	GR_COLOR	maxcolor;	/* maximum color used in pixmap */
	GR_COLOR	*colors;	/* table of color values */
	GR_BITMAP	*bitmap;	/* OR: table of bitmap values */
};


/*
 * Macros to obtain the client number from a resource id, and to
 * produce the first resource id to be used for a client number.
 * Client numbers must not be zero.  This allows for 255 clients.
#define	GR_ID_CLIENT(n)	(((GR_ID) (n)) >> 24)
#define	GR_ID_BASE(n)	(((GR_ID) (n)) << 24)
 */


/*
 * Graphics server routines.
 */
int		GsInitialize(void);
void		GsSelect(void);
void		GsTerminate(void);
int		GsOpen(void);
void		GsRedrawScreen(void);
void		GsFlush(void);
void		GsError(GR_ERROR code, GR_ID id);
GR_WINDOW_ID	GsNewWindow(GR_WINDOW_ID parent, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height, GR_SIZE bordersize,
			GR_COLOR background, GR_COLOR bordercolor);
GR_WINDOW_ID	GsNewInputWindow(GR_WINDOW_ID parent, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height);
void		GsDestroyWindow(GR_WINDOW_ID wid);
void		GsGetWindowInfo(GR_WINDOW_ID wid, GR_WINDOW_INFO *infoptr);
void		GsGetGCInfo(GR_GC_ID gcid, GR_GC_INFO *gcip);
void		GsGetFontInfo(GR_FONT font, GR_FONT_INFO *fip);
GR_GC_ID	GsNewGC(void);
GR_GC_ID	GsCopyGC(GR_GC_ID gc);
void		GsDestroyGC(GR_GC_ID gc);
void		GsSetGCForeground(GR_GC_ID gc, GR_COLOR foreground);
void		GsSetGCBackground(GR_GC_ID gc, GR_COLOR background);
void		GsSetGCUseBackground(GR_GC_ID gc, GR_BOOL flag);
void		GsSetGCMode(GR_GC_ID gc, GR_MODE mode);
void		GsSetGCFont(GR_GC_ID gc, GR_FONT font);
void		GsGetGCTextSize(GR_GC_ID gc, GR_CHAR *cp, GR_SIZE len,
			GR_SIZE *retwidth,GR_SIZE *retheight, GR_SIZE *retbase);
void		GsGetScreenInfo(GR_SCREEN_INFO *sip);
void		GsClearWindow(GR_WINDOW_ID wid, GR_BOOL exposeflag);
void		GsSelectEvents(GR_WINDOW_ID wid, GR_EVENT_MASK eventmask);
void		GsPoint(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y);
void		GsPoly(GR_DRAW_ID id, GR_GC_ID gc, GR_COUNT count,
			GR_POINT *pointtable);
void		GsFillPoly(GR_DRAW_ID id, GR_GC_ID gc, GR_COUNT count,
			GR_POINT *pointtable);
void		GsLine(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x1, GR_COORD y1,
			GR_COORD x2, GR_COORD y2);
void		GsEllipse(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
			GR_SIZE rx, GR_SIZE ry);
void		GsFillEllipse(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x,
			GR_COORD y, GR_SIZE rx, GR_SIZE ry);
void		GsRect(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height);
void		GsFillRect(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height);
void		GsBitmap(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height, GR_BITMAP *bitmaptable);
void		GsArea(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height, PIXELVAL *pixels);
void		GsReadArea(GR_DRAW_ID id, GR_COORD x, GR_COORD y,GR_SIZE width,
			GR_SIZE height, PIXELVAL *pixels);
void		GsText(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
			GR_CHAR *str, GR_COUNT count);
void		GsSetCursor(GR_WINDOW_ID wid, GR_SIZE width, GR_SIZE height,
			GR_COORD hotx, GR_COORD hoty, GR_COLOR foreground,
			GR_COLOR background, GR_BITMAP *fgbitmap,
			GR_BITMAP *bgbitmap);
void		GsMoveCursor(GR_COORD x, GR_COORD y);
void		GsMapWindow(GR_WINDOW_ID wid);
void		GsUnmapWindow(GR_WINDOW_ID wid);
void		GsRaiseWindow(GR_WINDOW_ID wid);
void		GsLowerWindow(GR_WINDOW_ID wid);
void		GsMoveWindow(GR_WINDOW_ID wid, GR_COORD x, GR_COORD y);
void		GsResizeWindow(GR_WINDOW_ID wid, GR_SIZE width, GR_SIZE height);
void		GsSetFocus(GR_WINDOW_ID wid);
void		GsSetBorderColor(GR_WINDOW_ID wid, GR_COLOR color);
void		GsRegisterInput(int fd);
void		GsGetNextEvent(GR_EVENT *ep);
void		GsCheckNextEvent(GR_EVENT *ep);
int		GsPeekEvent(GR_EVENT *ep);
void		GsCheckMouseEvent(void);
void		GsCheckKeyboardEvent(void);
int		GsReadKeyboard(GR_CHAR *buf, MODIFIER *modifiers);
int		GsOpenKeyboard(void);
void		GsGetButtonInfo(BUTTON *buttons);
void		GsGetModifierInfo(MODIFIER *modifiers);
void		GsClose(void);
void		GsCloseKeyboard(void);
void		GsExposeArea(GR_WINDOW *wp, GR_COORD rootx, GR_COORD rooty,
			GR_SIZE width, GR_SIZE height);
void		GsCheckCursor(void);
void		GsWpSetFocus(GR_WINDOW *wp);
void		GsWpClearWindow(GR_WINDOW *wp, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height, GR_BOOL exposeflag);
void		GsWpUnmapWindow(GR_WINDOW *wp);
void		GsWpMapWindow(GR_WINDOW *wp);
void		GsWpDestroyWindow(GR_WINDOW *wp);
void		GsSetClipWindow(GR_WINDOW *wp);
GR_COUNT	GsSplitClipRect(CLIPRECT *srcrect, CLIPRECT *destrect,
			GR_COORD minx, GR_COORD miny, GR_COORD maxx,
			GR_COORD maxy);
void		GsHandleMouseStatus(GR_COORD newx, GR_COORD newy,
			BUTTON newbuttons);
void		GsFreePositionEvent(GR_CLIENT *client, GR_WINDOW_ID wid,
			GR_WINDOW_ID subwid);
void		GsDeliverButtonEvent(GR_EVENT_TYPE type, BUTTON buttons,
			BUTTON changebuttons, MODIFIER modifiers);
void		GsDeliverMotionEvent(GR_EVENT_TYPE type, BUTTON buttons,
			MODIFIER modifiers);
void		GsDeliverKeyboardEvent(GR_EVENT_TYPE type, GR_CHAR ch,
			MODIFIER modifiers);
void		GsDeliverExposureEvent(GR_WINDOW *wp, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height);
void		GsDeliverGeneralEvent(GR_WINDOW *wp, GR_EVENT_TYPE type);
void		GsCheckMouseWindow(void);
void		GsCheckFocusWindow(void);
GR_DRAW_TYPE	GsPrepareDrawing(GR_DRAW_ID id, GR_GC_ID gcid,
			GR_WINDOW **retwp, GR_PIXMAP **retpixptr);
GR_BOOL		GsCheckOverlap(GR_WINDOW *topwp, GR_WINDOW *botwp);
GR_EVENT	*GsAllocEvent(GR_CLIENT *client);
GR_WINDOW	*GsFindWindow(GR_WINDOW_ID id);
GR_GC		*GsFindGC(GR_GC_ID gcid);
GR_WINDOW	*GsPrepareWindow(GR_WINDOW_ID wid);
GR_WINDOW	*GsFindVisibleWindow(GR_COORD x, GR_COORD y);
void		GsDrawBorder(GR_WINDOW *wp);
int		GsCurrentVt(void);
void		GsRedrawVt(int t);
int		GsOpenSocket(void);
void		GsCloseSocket(void);
void		GsAcceptClient(void);
void		GsAcceptClientFd(int i);
int		GsPutCh(int fd, unsigned char c);
GR_CLIENT	*GsFindClient(int fd);
void		GsDropClient(int fd);
int		GsRead(int fd, void *buf, int c);
int		GsWrite(int fd, void *buf, int c);
void		GsHandleClient(int fd);
void		GsOpenWrapper(void);
void		GsCloseWrapper(void);
void		GsGetScreenInfoWrapper(void);
void		GsSetErrorHandlerWrapper(void);
void		GsNewWindowWrapper(void);
void		GsNewInputWindowWrapper(void);
void		GsDestroyWindowWrapper(void);
void		GsNewGCWrapper(void);
void		GsGetGCInfoWrapper(void);
void		GsDestroyGCWrapper(void);
void		GsMapWindowWrapper(void);
void		GsUnmapWindowWrapper(void);
void		GsRaiseWindowWrapper(void);
void		GsLowerWindowWrapper(void);
void		GsMoveWindowWrapper(void);
void		GsResizeWindowWrapper(void);
void		GsGetWindowInfoWrapper(void);
void		GsGetFontInfoWrapper(void);
void		GsSetFocusWrapper(void);
void		GsSetBorderColorWrapper(void);
void		GsClearWindowWrapper(void);
void		GsSelectEventsWrapper(void);
void		GsGetNextEventWrapper(void);
void		GsGetNextEventWrapperFinish(void);
void		GsCheckNextEventWrapper(void);
void		GsPeekEventWrapper(void);
void		GsFlushWrapper(void);
void		GsLineWrapper(void);
void		GsPointWrapper(void);
void		GsRectWrapper(void);
void		GsFillRectWrapper(void);
void		GsPolyWrapper(void);
void		GsFillPolyWrapper(void);
void		GsEllipseWrapper(void);
void		GsFillEllipseWrapper(void);
void		GsSetGCForegroundWrapper(void);
void		GsSetGCBackgroundWrapper(void);
void		GsSetGCUseBackgroundWrapper(void);
void		GsSetGCModeWrapper(void);
void		GsSetGCFontWrapper(void);
void		GsGetGCTextSizeWrapper(void);
void		GsReadArea8Wrapper(void);
void		GsArea8Wrapper(void);
void		GsBitmapWrapper(void);
void		GsTextWrapper(void);
void		GsSetCursorWrapper(void);
void		GsMoveCursorWrapper(void);

/*
 * External data definitions.
 */
extern	char *		curfunc;		/* current function name */
extern	GR_WINDOW_ID	cachewindowid;		/* cached window id */
extern	GR_GC_ID	cachegcid;		/* cached graphics context id */
extern	GR_GC		*cachegcp;		/* cached graphics context */
extern	GR_GC		*listgcp;		/* list of all gc */
extern	GR_GC		*curgcp;		/* current graphics context */
extern	GR_WINDOW	*cachewp;		/* cached window pointer */
extern	GR_WINDOW	*listwp;		/* list of all windows */
extern	GR_WINDOW	*rootwp;		/* root window pointer */
extern	GR_WINDOW	*clipwp;		/* window clipping is set for */
extern	GR_WINDOW	*focuswp;		/* focus window for keyboard */
extern	GR_WINDOW	*mousewp;		/* window mouse is currently in */
extern	GR_WINDOW	*grabbuttonwp;		/* window grabbed by button */
extern	GR_CURSOR	*curcursor;		/* currently enabled cursor */
extern	GR_COORD	cursorx;		/* x position of cursor */
extern	GR_COORD	cursory;		/* y position of cursor */
extern	BUTTON	curbuttons;			/* current state of buttons */
extern	GR_CLIENT	*curclient;		/* current client */
extern	GR_EVENT_LIST	*eventfree;		/* list of free events */
extern	GR_BOOL		focusfixed;		/* TRUE if focus is fixed */
extern	GR_SCREEN_INFO	sinfo;			/* screen information */
extern	GR_FONT_INFO	curfont;		/* current font information */

/*
 * The filename to use for the named socket. If we ever support multiple
 * servers on one machine, the last digit will be that of the FB used for it.
 */
#define GR_NAMED_SOCKET "/var/uds"

/*
 * The network interface version number. Increment this if you make a change
 * to the interface  which will break old clients. It can only be up to 256.
 */
#define GR_INTERFACE_NUMBER 0

/*
 * The server magic word, which clients expect to read just prior to the interface
 * version number when they connect.
 */
#define GrServerMagic "NanoXserver"

/*
 * The client magic word, which the server expects to read after it has sent the
 * server magic word and interface version number to the client.
 */
#define GrClientMagic "NanoXclient"

/*
 * The function numbers which the client sends to the server to execute the
 * relevant calls on the server. If you add a new one, add it before the last
 * entry and increment the number of the last entry appropriately. Also remember
 * to add your function to the array in serv_net.c, to put a stub function in
 * client.c, and put the function itself in serv_net.c.
 * There can be no more than 256 functions.
 */

#define GrNumOpen               0
#define GrNumClose              1
#define GrNumGetScreenInfo      2
#define GrNumNewWindow          3
#define GrNumNewInputWindow     4
#define GrNumDestroyWindow      5
#define GrNumNewGC              6
#define GrNumCopyGC		7
#define GrNumGetGCInfo          8
#define GrNumDestroyGC          9
#define GrNumMapWindow          10
#define GrNumUnmapWindow        11
#define GrNumRaiseWindow        12
#define GrNumLowerWindow        13
#define GrNumMoveWindow         14
#define GrNumResizeWindow       15
#define GrNumGetWindowInfo      16
#define GrNumGetFontInfo        17
#define GrNumSetFocus           18
#define GrNumSetBorderColor     19
#define GrNumClearWindow        20
#define GrNumSelectEvents       21
#define GrNumGetNextEvent       22
#define GrNumCheckNextEvent     23
#define GrNumPeekEvent          24
#define GrNumFlush              25
#define GrNumLine               26
#define GrNumPoint              27
#define GrNumRect               28
#define GrNumFillRect           29
#define GrNumPoly               30
#define GrNumFillPoly           31
#define GrNumEllipse            32
#define GrNumFillEllipse        33
#define GrNumSetGCForeground    34
#define GrNumSetGCBackground    35
#define GrNumSetGCUseBackground 36
#define GrNumSetGCMode          37
#define GrNumSetGCFont          38
#define GrNumGetGCTextSize      39
#define GrNumReadArea           40
#define GrNumArea               41
#define GrNumBitmap             42
#define GrNumText               43
#define GrNumSetCursor          44
#define GrNumMoveCursor         45
#define GrTotalNumCalls         46

/*
 * The values the server can return in response to a command.
 * Items marked with a * are transitory- ie. if the client gets one of these it should try
 * again after carrying out the action it specifies. There are never more than one of each
 * of these in any one transaction, and the lowest numbered ones are sent first.
 */
#define GrRetOK			1	/* Ok, nothing more to do. */
#define GrRetSendData		2	/* Ok, send the data associated with the function. */
#define GrRetDataFollows	3	/* Ok, data follows (you already know the length). */
#define GrRetDataFollows1	4	/* Ok, data follows (starting with 1 byte length value). */
#define GrRetDataFollows2	5	/* Ok, data follows (starting with 2 byte length value). */
#define GrRetDataFollows4	6	/* Ok, data follows (starting with 4 byte length value). */
#define GrRetENoFunction	7	/* That function number is invalid. */
#define GrRetEInval		8	/* The function arguments were invalid. */
#define GrRetENoMem		9	/* Didn't have enough memory to carry it out. */
#define GrRetEPerm		10	/* Permission denied */
#define GrRetESig		11	/* Send yourself a signal * */
#define GrRetErrorPending	12	/* An error signal is waiting for you * */

#if FRAMEBUFFER | BOGL
/* temp framebuffer vt switch stuff at upper level
 * this should be handled at the lower level, just like vgalib does.
 */
void fb_InitVt(void);
int  fb_CurrentVt(void);
int  fb_CheckVtChange(void);
void fb_RedrawVt(int t);
void fb_ExitVt(void);
extern int vterm;
#endif /* FRAMEBUFFER | BOGL*/

#endif /* _SERV_H*/
