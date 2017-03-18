#ifndef	_NANO_X_H
#define	_NANO_X_H
/* Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * nano-X public definition header file:  user applications should
 * include only this header file.
 */
#include "device.h"

/* The following typedefs are inherited from the mid-level
 * device driver layer. Thus, there size may change on different
 * systems nano-X is ported to.
 */
typedef COORD 		GR_COORD;	/* coordinate value */
typedef COORD 		GR_SIZE;	/* size value */
typedef int 		GR_COUNT;	/* number of items */
typedef COLORVAL 	GR_COLOR;	/* full color value */
typedef IMAGEBITS 	GR_BITMAP;	/* bitmap unit */
typedef MODE 		GR_MODE;	/* drawing mode */
typedef UCHAR 		GR_CHAR;	/* text character */
typedef SCREENINFO	GR_SCREEN_INFO;	/* screen information*/
typedef FONTINFO	GR_FONT_INFO;	/* font information*/

/* Basic typedefs. */
typedef unsigned char GR_CHAR_WIDTH;	/* width of character */
typedef unsigned int GR_ID;	/* resource ids */
typedef GR_ID GR_DRAW_ID;	/* drawable id */
typedef GR_DRAW_ID GR_WINDOW_ID;/* window id */
typedef GR_DRAW_ID GR_PIXMAP_ID;/* pixmap id (not yet used) */
typedef GR_ID GR_GC_ID;		/* graphics context id */
typedef unsigned short GR_FONT;	/* font number */
typedef unsigned short GR_BOOL;	/* boolean value */
typedef unsigned short GR_FUNC;	/* function codes */
typedef int GR_ERROR;		/* error value */
typedef short GR_EVENT_TYPE;	/* event types */
typedef unsigned long GR_EVENT_MASK;	/* event masks */
typedef char GR_FUNC_NAME[20];	/* function name */
typedef void (*GR_ERROR_FUNC)();	/* error function */

/* Pixel packings within words. */
#define	GR_BITMAPBITS	(sizeof(GR_BITMAP) * 8)
#define	GR_ZEROBITS	((GR_BITMAP) 0x0000)
#define	GR_ONEBITS	((GR_BITMAP) 0xffff)
#define	GR_FIRSTBIT	((GR_BITMAP) 0x8000)
#define	GR_LASTBIT	((GR_BITMAP) 0x0001)
#define	GR_BITVALUE(n)	((GR_BITMAP) (((GR_BITMAP) 1) << (n)))
#define	GR_SHIFTBIT(m)	((GR_BITMAP) ((m) << 1))
#define	GR_NEXTBIT(m)	((GR_BITMAP) ((m) >> 1))
#define	GR_TESTBIT(m)	(((m) & GR_FIRSTBIT) != 0)

/* Size of bitmaps. */
#define	GR_BITMAP_SIZE(width, height)	((height) * \
  (((width) + sizeof(GR_BITMAP) * 8 - 1) / (sizeof(GR_BITMAP) * 8)))

#define	GR_MAX_BITMAP_SIZE \
  GR_BITMAP_SIZE(MAX_CURSOR_SIZE, MAX_CURSOR_SIZE)

/* The root window id. */
#define	GR_ROOT_WINDOW_ID	((GR_WINDOW_ID) 1)

/* Drawing mode (used with GrSetGCMode). */
#define	GR_MODE_SET	((GR_MODE)MODE_SET) /* draw pixels as given (default) */
#define	GR_MODE_XOR	((GR_MODE)MODE_XOR)	/* draw pixels using XOR */
#define	GR_MODE_OR	((GR_MODE)MODE_OR)	/* draw pixels using OR */
#define	GR_MODE_AND	((GR_MODE)MODE_AND)	/* draw pixels using AND */
#define	GR_MAX_MODE	((GR_MODE)MODE_AND)	/* maximum legal mode */

/* Booleans. */
#define	GR_FALSE	((GR_BOOL) 0)
#define	GR_TRUE		((GR_BOOL) 1)

/* Definition of a point. */
typedef XYPOINT	GR_POINT;

/* Window properties returned by the GrGetWindowInfo call. */
typedef struct {
  GR_WINDOW_ID wid;		/* window id (or 0 if no such window) */
  GR_WINDOW_ID parent;		/* parent window id */
  GR_WINDOW_ID child;		/* first child window id (or 0) */
  GR_WINDOW_ID sibling;		/* next sibling window id (or 0) */
  GR_BOOL inputonly;		/* TRUE if window is input only */
  GR_BOOL mapped;		/* TRUE if window is mapped */
  GR_COUNT unmapcount;		/* reasons why window is unmapped */
  GR_COORD x;			/* absolute x position of window */
  GR_COORD y;			/* absolute y position of window */
  GR_SIZE width;		/* width of window */
  GR_SIZE height;		/* height of window */
  GR_SIZE bordersize;		/* size of border */
  GR_COLOR bordercolor;		/* color of border */
  GR_COLOR background;		/* background color */
  GR_EVENT_MASK eventmask;	/* current event mask for this client */
} GR_WINDOW_INFO;

/* Graphics context properties returned by the GrGetGCInfo call. */
typedef struct {
  GR_GC_ID gcid;		/* GC id (or 0 if no such GC) */
  GR_MODE mode;			/* drawing mode */
  GR_FONT font;			/* font number */
  GR_COLOR foreground;		/* foreground color */
  GR_COLOR background;		/* background color */
  GR_BOOL usebackground;	/* use background in bitmaps */
} GR_GC_INFO;

/* Error codes */
#define	GR_ERROR_BAD_WINDOW_ID		((GR_ERROR) 1)
#define	GR_ERROR_BAD_GC_ID		((GR_ERROR) 2)
#define	GR_ERROR_BAD_CURSOR_SIZE	((GR_ERROR) 3)
#define	GR_ERROR_MALLOC_FAILED		((GR_ERROR) 4)
#define	GR_ERROR_BAD_WINDOW_SIZE	((GR_ERROR) 5)
#define	GR_ERROR_KEYBOARD_ERROR		((GR_ERROR) 6)
#define	GR_ERROR_MOUSE_ERROR		((GR_ERROR) 7)
#define	GR_ERROR_INPUT_ONLY_WINDOW	((GR_ERROR) 8)
#define	GR_ERROR_ILLEGAL_ON_ROOT_WINDOW	((GR_ERROR) 9)
#define	GR_ERROR_TOO_MUCH_CLIPPING	((GR_ERROR) 10)
#define	GR_ERROR_SCREEN_ERROR		((GR_ERROR) 11)
#define	GR_ERROR_UNMAPPED_FOCUS_WINDOW	((GR_ERROR) 12)
#define	GR_ERROR_BAD_DRAWING_MODE	((GR_ERROR) 13)

/* Event types.
 * Mouse motion is generated for every motion of the mouse, and is used to
 * track the entire history of the mouse (many events and lots of overhead).
 * Mouse position ignores the history of the motion, and only reports the
 * latest position of the mouse by only queuing the latest such event for
 * any single client (good for rubber-banding).
 */
#define	GR_EVENT_TYPE_ERROR		((GR_EVENT_TYPE) -1)
#define	GR_EVENT_TYPE_NONE		((GR_EVENT_TYPE) 0)
#define	GR_EVENT_TYPE_EXPOSURE		((GR_EVENT_TYPE) 1)
#define	GR_EVENT_TYPE_BUTTON_DOWN	((GR_EVENT_TYPE) 2)
#define	GR_EVENT_TYPE_BUTTON_UP		((GR_EVENT_TYPE) 3)
#define	GR_EVENT_TYPE_MOUSE_ENTER	((GR_EVENT_TYPE) 4)
#define	GR_EVENT_TYPE_MOUSE_EXIT	((GR_EVENT_TYPE) 5)
#define	GR_EVENT_TYPE_MOUSE_MOTION	((GR_EVENT_TYPE) 6)
#define	GR_EVENT_TYPE_MOUSE_POSITION	((GR_EVENT_TYPE) 7)
#define	GR_EVENT_TYPE_KEY_DOWN		((GR_EVENT_TYPE) 8)
#define	GR_EVENT_TYPE_KEY_UP		((GR_EVENT_TYPE) 9)
#define	GR_EVENT_TYPE_FOCUS_IN		((GR_EVENT_TYPE) 10)
#define	GR_EVENT_TYPE_FOCUS_OUT		((GR_EVENT_TYPE) 11)
#define GR_EVENT_TYPE_FDINPUT		((GR_EVENT_TYPE) 12)

/* Event masks */
#define	GR_EVENTMASK(n)			(((GR_EVENT_MASK) 1) << (n))

#define	GR_EVENT_MASK_NONE		GR_EVENTMASK(GR_EVENT_TYPE_NONE)
#define	GR_EVENT_MASK_ERROR		GR_EVENTMASK(GR_EVENT_TYPE_ERROR)
#define	GR_EVENT_MASK_EXPOSURE		GR_EVENTMASK(GR_EVENT_TYPE_EXPOSURE)
#define	GR_EVENT_MASK_BUTTON_DOWN	GR_EVENTMASK(GR_EVENT_TYPE_BUTTON_DOWN)
#define	GR_EVENT_MASK_BUTTON_UP		GR_EVENTMASK(GR_EVENT_TYPE_BUTTON_UP)
#define	GR_EVENT_MASK_MOUSE_ENTER	GR_EVENTMASK(GR_EVENT_TYPE_MOUSE_ENTER)
#define	GR_EVENT_MASK_MOUSE_EXIT	GR_EVENTMASK(GR_EVENT_TYPE_MOUSE_EXIT)
#define	GR_EVENT_MASK_MOUSE_MOTION	GR_EVENTMASK(GR_EVENT_TYPE_MOUSE_MOTION)
#define	GR_EVENT_MASK_MOUSE_POSITION	GR_EVENTMASK(GR_EVENT_TYPE_MOUSE_POSITION)
#define	GR_EVENT_MASK_KEY_DOWN		GR_EVENTMASK(GR_EVENT_TYPE_KEY_DOWN)
#define	GR_EVENT_MASK_KEY_UP		GR_EVENTMASK(GR_EVENT_TYPE_KEY_UP)
#define	GR_EVENT_MASK_FOCUS_IN		GR_EVENTMASK(GR_EVENT_TYPE_FOCUS_IN)
#define	GR_EVENT_MASK_FOCUS_OUT		GR_EVENTMASK(GR_EVENT_TYPE_FOCUS_OUT)
#define	GR_EVENT_MASK_ALL		((GR_EVENT_MASK) -1L)

/* Modifiers generated by special keyboard shift-like keys.
 * The state of these keys can be read as up or down, and don't
 * generate any characters by themselves.
 */
#define	GR_MODIFIER_SHIFT	((MODIFIER) 0x01)	/* shift */
#define	GR_MODIFIER_CTRL	((MODIFIER) 0x02)	/* control */
#define	GR_MODIFIER_META	((MODIFIER) 0x03)	/* meta or alt */
#define	GR_MODIFIER_ANY		((MODIFIER) 0x07)	/* any modifier */

/* Button flags. */
#define	GR_BUTTON_R	((BUTTON) RBUTTON) 	/* right button*/
#define	GR_BUTTON_M	((BUTTON) MBUTTON)	/* middle button*/
#define	GR_BUTTON_L	((BUTTON) LBUTTON)	/* left button */
#define	GR_BUTTON_ANY	((BUTTON) RBUTTON|MBUTTON|LBUTTON) /* any button*/

/* Event for errors detected by the server.
 * These events are not delivered to GrGetNextEvent, but instead call
 * the user supplied error handling function.  Only the first one of
 * these errors at a time is saved for delivery to the client since
 * there is not much to be done about errors anyway except complain
 * and exit.
 */
typedef struct {
  GR_EVENT_TYPE type;		/* event type */
  GR_FUNC_NAME name;		/* function name which failed */
  GR_ERROR code;		/* error code */
  GR_ID id;			/* resource id (maybe useless) */
} GR_EVENT_ERROR;

/* Event for a mouse button pressed down or released. */
typedef struct {
  GR_EVENT_TYPE type;		/* event type */
  GR_WINDOW_ID wid;		/* window id event delivered to */
  GR_WINDOW_ID subwid;		/* sub-window id (pointer was in) */
  GR_COORD rootx;		/* root window x coordinate */
  GR_COORD rooty;		/* root window y coordinate */
  GR_COORD x;			/* window x coordinate of mouse */
  GR_COORD y;			/* window y coordinate of mouse */
  BUTTON buttons;		/* current state of all buttons */
  BUTTON changebuttons;		/* buttons which went down or up */
  MODIFIER modifiers;		/* modifiers (SHIFT, CTRL, etc) */
} GR_EVENT_BUTTON;

/* Event for a keystroke typed for the window with has focus. */
typedef struct {
  GR_EVENT_TYPE type;		/* event type */
  GR_WINDOW_ID wid;		/* window id event delived to */
  GR_WINDOW_ID subwid;		/* sub-window id (pointer was in) */
  GR_COORD rootx;		/* root window x coordinate */
  GR_COORD rooty;		/* root window y coordinate */
  GR_COORD x;			/* window x coordinate of mouse */
  GR_COORD y;			/* window y coordinate of mouse */
  BUTTON buttons;		/* current state of buttons */
  MODIFIER modifiers;		/* modifiers (SHIFT, CTRL, etc) */
  GR_CHAR ch;			/* keystroke or function key */
} GR_EVENT_KEYSTROKE;

/* Event for exposure for a region of a window. */
typedef struct {
  GR_EVENT_TYPE type;		/* event type */
  GR_WINDOW_ID wid;		/* window id */
  GR_COORD x;			/* window x coordinate of exposure */
  GR_COORD y;			/* window y coordinate of exposure */
  GR_SIZE width;		/* width of exposure */
  GR_SIZE height;		/* height of exposure */
} GR_EVENT_EXPOSURE;

/* General events for focus in or focus out for a window, or mouse enter
 * or mouse exit from a window, or window unmapping or mapping, etc.
 */
typedef struct {
  GR_EVENT_TYPE type;		/* event type */
  GR_WINDOW_ID wid;		/* window id */
} GR_EVENT_GENERAL;

/* Events for mouse motion or mouse position. */
typedef struct {
  GR_EVENT_TYPE type;		/* event type */
  GR_WINDOW_ID wid;		/* window id event delivered to */
  GR_WINDOW_ID subwid;		/* sub-window id (pointer was in) */
  GR_COORD rootx;		/* root window x coordinate */
  GR_COORD rooty;		/* root window y coordinate */
  GR_COORD x;			/* window x coordinate of mouse */
  GR_COORD y;			/* window y coordinate of mouse */
  BUTTON buttons;		/* current state of buttons */
  MODIFIER modifiers;		/* modifiers (ALT, SHIFT, etc) */
} GR_EVENT_MOUSE;

/* GrRegisterInput event*/
typedef struct {
  GR_EVENT_TYPE type;		/* event type */
  int		fd;		/* input fd*/
} GR_EVENT_FDINPUT;

/* Union of all possible event structures.
 * This is the structure returned by the GrGetNextEvent and similar routines.
 */
typedef union {
  GR_EVENT_TYPE type;		/* event type */
  GR_EVENT_ERROR error;		/* error event */
  GR_EVENT_GENERAL general;	/* general window events */
  GR_EVENT_BUTTON button;	/* button events */
  GR_EVENT_KEYSTROKE keystroke;	/* keystroke events */
  GR_EVENT_EXPOSURE exposure;	/* exposure events */
  GR_EVENT_MOUSE mouse;		/* mouse motion events */
  GR_EVENT_FDINPUT fdinput;	/* fd input events*/
} GR_EVENT;

/* Public graphics routines. */
int		GrOpen(void);
int		GrClose(void);
int		GrGetScreenInfo(GR_SCREEN_INFO *sip);
void		GrDefaultErrorHandler(GR_EVENT_ERROR err);
GR_ERROR_FUNC	GrSetErrorHandler(GR_ERROR_FUNC func);
GR_WINDOW_ID	GrNewWindow(GR_WINDOW_ID parent, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height, GR_SIZE bordersize,
			GR_COLOR background, GR_COLOR bordercolor);
GR_WINDOW_ID	GrNewInputWindow(GR_WINDOW_ID parent, GR_COORD x, GR_COORD y,
				GR_SIZE width, GR_SIZE height);
int		GrDestroyWindow(GR_WINDOW_ID wid);
GR_GC_ID	GrNewGC(void);
int		GrGetGCInfo(GR_GC_ID gc, GR_GC_INFO *gcip);
int		GrDestroyGC(GR_GC_ID gc);
int		GrMapWindow(GR_WINDOW_ID wid);
int		GrUnmapWindow(GR_WINDOW_ID wid);
int		GrRaiseWindow(GR_WINDOW_ID wid);
int		GrLowerWindow(GR_WINDOW_ID wid);
int		GrMoveWindow(GR_WINDOW_ID wid, GR_COORD x, GR_COORD y);
int		GrResizeWindow(GR_WINDOW_ID wid, GR_SIZE width, GR_SIZE height);
int		GrGetWindowInfo(GR_WINDOW_ID wid, GR_WINDOW_INFO *infoptr);
int		GrGetFontInfo(GR_FONT font, GR_FONT_INFO *fip);
int		GrSetFocus(GR_WINDOW_ID wid);
int		GrSetBorderColor(GR_WINDOW_ID wid, GR_COLOR color);
int		GrClearWindow(GR_WINDOW_ID wid, GR_BOOL exposeflag);
int		GrSelectEvents(GR_WINDOW_ID wid, GR_EVENT_MASK eventmask);
int		GrRegisterInput(int fd);
int		GrGetNextEvent(GR_EVENT *ep);
int		GrCheckNextEvent(GR_EVENT *ep);
int		GrPeekEvent(GR_EVENT *ep);
int		GrFlush(void);
int		GrLine(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x1, GR_COORD y1,
			GR_COORD x2, GR_COORD y2);
int		GrPoint(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y);
int		GrRect(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height);
int		GrFillRect(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height);
int		GrPoly(GR_DRAW_ID id, GR_GC_ID gc, GR_COUNT count,
			GR_POINT *pointtable);
int		GrFillPoly(GR_DRAW_ID id, GR_GC_ID gc, GR_COUNT count,
			GR_POINT *pointtable);
int		GrEllipse(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
			GR_SIZE rx, GR_SIZE ry);
int		GrFillEllipse(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x,
			GR_COORD y, GR_SIZE rx, GR_SIZE ry);
int		GrSetGCForeground(GR_GC_ID gc, GR_COLOR foreground);
int		GrSetGCBackground(GR_GC_ID gc, GR_COLOR background);
int		GrSetGCUseBackground(GR_GC_ID gc, GR_BOOL flag);
int		GrSetGCMode(GR_GC_ID gc, GR_MODE mode);
int		GrSetGCFont(GR_GC_ID gc, GR_FONT font);
int		GrGetGCTextSize(GR_GC_ID gc, GR_CHAR *cp, GR_SIZE len,
			GR_SIZE *retwidth, GR_SIZE *retheight,GR_SIZE *retbase);
int		GrReadArea(GR_DRAW_ID id, GR_COORD x, GR_COORD y, GR_SIZE width,
			GR_SIZE height, PIXELVAL *pixels);
int		GrArea(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height, PIXELVAL *pixels);
int		GrBitmap(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
			GR_SIZE width, GR_SIZE height, GR_BITMAP *imagebits);
int		GrText(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y,
			GR_CHAR *str, GR_COUNT count);
int		GrSetCursor(GR_WINDOW_ID wid, GR_SIZE width, GR_SIZE height,
			GR_COORD hotx, GR_COORD hoty, GR_COLOR foreground,
			GR_COLOR background, GR_BITMAP *fbbitmap,
			GR_BITMAP *bgbitmap);
int		GrMoveCursor(GR_COORD x, GR_COORD y);

extern	int	GrErrno;
#endif /* _NANO_X_H*/
