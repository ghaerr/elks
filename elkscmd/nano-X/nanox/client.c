/*
 * Copyright (c) 1999 Alex Holden <alex@linuxhacker.org>
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Client routines to do graphics with windows and graphics contexts.
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#ifndef __linux__ 
#include <linuxmt/socket.h>
#include <linuxmt/un.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#endif
#include "nano-X.h"
#include "serv.h"

static int sock;	/* The network socket descriptor */
int	GrErrno;	/* The Nano-X equivalent of errno */

/*
 * The following is the user defined function for handling errors.
 * If this is not set, then the default action is to close the connection
 * to the server, describe the error, and then exit.  This error function
 * will only be called when the client asks for events.
 */
#ifdef __linux__ 
static GR_ERROR_FUNC GrErrorFunc = &GrDefaultErrorHandler;
#endif
/*
 * Read n bytes of data from the server into block *b.
 * Returns 0 on success or -1 on failure.
 */
static int GrReadBlock(void *b, int n)
{
	int i = 0;
	char *v;

	v = (char *) b;

	while(v < ((char *) b + n)) {
		i = read(sock, v, ((char *) b + n - v));
		if(i <= 0) return -1;
		v += i;
	}

	return 0;
}

/*
 * Read a byte of data from the server.
 */
static int GrReadByte()
{
	unsigned char c;
	if(GrReadBlock(&c, 1) == -1)
		return -1;
	else return (int) c;
}

/*
 * Read an error event structure from the server and deliver it to the client.
 */
static int GrDeliverErrorEvent(void)
{
	GR_EVENT_ERROR err;

	if(GrReadBlock(&err, sizeof(err)) == -1)
		return -1;
#ifndef __linux__ 
	GrDefaultErrorHandler(err);
#else	
	GrErrorFunc(err);
#endif	

	return 0;
}

/*
 * Send a block of data to the server and read it's reply.
 */
static int GrSendBlock(void *b, long n)
{
	int i = 0, z;
	unsigned char *c;

	c = (unsigned char *) b;

	/* FIXME: n > 64k will fail here if sizeof(int) == 2*/
	while(c < ((unsigned char *) b + n)) {
		i = write(sock, c, ((unsigned char *) b + n - c));
		if(i <= 0) return -1;
		c += i;
	}

	do {
		if((i = GrReadByte()) < 0) return -1;
		else if(i == GrRetESig) {
			z = GrReadByte();
			if(z == -1) return -1;
printf("client bad GrSendBlock\r\n");
			raise(z);
		}
		else if(i == GrRetErrorPending)
			if(GrDeliverErrorEvent() == -1) return -1;
			
	} while((i == GrRetESig) | (i == GrRetErrorPending));

	return((int) i);
}

/*
 * Send a single byte to the server and read the reply.
 */
static int GrSendByte(unsigned char c)
{
	return GrSendBlock(&c, 1);
}

/*
 * Open a connection to the graphics server.
 * Returns the fd of the connection to the server if successful, or -1 on failure.
 */
int GrOpen(void)
{
	struct sockaddr_un name;
	size_t size;

	
	if(!sock)
		if((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
			sock = 0;
			return -1;
		}

	name.sun_family = AF_UNIX;
	strcpy(name.sun_path, GR_NAMED_SOCKET);
	size = sizeof(name); //(offsetof(struct sockaddr_un, sun_path) + strlen(name.sun_path) + 1);
	if(connect(sock, (struct sockaddr *) &name, size) == -1)
		return -1;

	if(GrSendByte(GrNumOpen) != GrRetOK)
		return -1;

	return sock;
}

/*
 * Close the graphics device, first flushing any waiting messages.
 */
int GrClose(void)
{
	GrSendByte(GrNumClose);
	close(sock);

	return 0;
}

/* 
 * The default error handler which is called when the server reports an error event
 * and the client hasn't set a handler for error events.
 */
void GrDefaultErrorHandler(GR_EVENT_ERROR err)
{
	char *why;

	switch(err.code) {
		case GR_ERROR_BAD_WINDOW_ID:
			why = "it was passed a bad window id";
			break;
		case GR_ERROR_BAD_GC_ID:
			why = "it was passed a bad graphics context id";
			break;
		case GR_ERROR_BAD_CURSOR_SIZE:
			why = "it was passed a bad cursor size";
			break;
		case GR_ERROR_MALLOC_FAILED:
			why = "the server is running out of memory";
			break;
		case GR_ERROR_BAD_WINDOW_SIZE:
			why = "it was passed a bad window size";
			break;
		case GR_ERROR_KEYBOARD_ERROR:
			why = "there was a keyboard error";
			break;
		case GR_ERROR_MOUSE_ERROR:
			why = "there was a mouse error";
			break;
		case GR_ERROR_INPUT_ONLY_WINDOW:
			why = "it was told to output to an input only window";
			break;
		case GR_ERROR_ILLEGAL_ON_ROOT_WINDOW:
			why = "it was asked to perform an action which is illegal to "
				"perform on the root window";
			break;
		case GR_ERROR_TOO_MUCH_CLIPPING:
			why = "it was asked to clip too far";
			break;
		case GR_ERROR_SCREEN_ERROR:
			why = "there was a screen error";
		case GR_ERROR_UNMAPPED_FOCUS_WINDOW:
			why = "it was asked to switch the focus to an unmapped window";
			break;
		case GR_ERROR_BAD_DRAWING_MODE:
			why = "it was asked to switch to a bad drawing mode";
			break;
		default:
			why = "the moon is in the wrong phase";
			break;
	}

	fprintf(stderr,"Error event recieved from server:\n"
		"\t%s() failed because %s.\n", err.name, why);
}

/*
 * Set an error handling routine, which will be called on any errors from
 * the server (when events are asked for by the client).  If zero is given,
 * then a default routine will be used.  Returns the previous error handler.
 */
GR_ERROR_FUNC GrSetErrorHandler(GR_ERROR_FUNC func)
{
#ifndef __linux__ 
	GR_ERROR_FUNC temp = GrDefaultErrorHandler;
#else  
	GR_ERROR_FUNC temp = GrErrorFunc;
	if(!func) GrErrorFunc = &GrDefaultErrorHandler;
	else GrErrorFunc = func;
#endif	
	return temp;
}

/*
 * Return useful information about the screen.
 * Returns -1 on failure or 0 on success.
 */
int GrGetScreenInfo(GR_SCREEN_INFO *sip)
{
	if(GrSendByte(GrNumGetScreenInfo) != GrRetDataFollows)
		return -1;

	if(GrReadBlock(sip, sizeof(GR_SCREEN_INFO)) == -1)
		return -1;

	return 0;
}

/*
 * Return useful information about the specified font.
 */
int GrGetFontInfo(GR_FONT fontno, GR_FONT_INFO *fip)
{
	if(GrSendByte(GrNumGetFontInfo) != GrRetSendData)
		return -1;

	if(GrSendBlock(&fontno, sizeof(fontno)) != GrRetDataFollows)
		return -1;

	if(GrReadBlock(fip, sizeof(GR_FONT_INFO)) == -1)
		return -1;

	return 0;
}

/*
 * Return information about the specified graphics context.
 */
int GrGetGCInfo(GR_GC_ID gc, GR_GC_INFO *gcip)
{
	if(GrSendByte(GrNumGetGCInfo) != GrRetSendData)
		return -1;

	if(GrSendBlock(&gc, sizeof(gc)) != GrRetDataFollows)
		return -1;

	if(GrReadBlock(gcip, sizeof(GR_GC_INFO)) == -1)
		return -1;

	return 0;
}

/*
 * Return the size of a text string for the font in a graphics context.
 * This is the width of the string, the height of the string,
 * and the height above the bottom of the font of the baseline for the font.
 */
int GrGetGCTextSize(GR_GC_ID gc, GR_CHAR *cp, GR_SIZE len, GR_SIZE *retwidth,
	GR_SIZE *retheight, GR_SIZE *retbase)
{
	if(GrSendByte(GrNumGetGCTextSize) != GrRetSendData)
		return -1;

	if(GrSendBlock(&gc, sizeof(gc)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&len, sizeof(len)) != GrRetSendData)
		return -1;

	if(GrSendBlock(cp, len) != GrRetDataFollows)
		return -1;

	if(GrReadBlock(retwidth, sizeof(*retwidth)) == -1)
		return -1;

	if(GrReadBlock(retheight, sizeof(*retheight)) == -1)
		return -1;

	if(GrReadBlock(retbase, sizeof(*retbase)) == -1)
		return -1;
	return 0;
}

/*
 * Register the specified file descriptor to return an event
 * when input is ready.
 * FIXME: only one external file descriptor works
 */
static int regfd = -1;

int GrRegisterInput(int fd)
{
	regfd = fd;
	return 0;
}

/*
 * Return the next event from the event queue.
 * This waits for a new one if one is not ready.
 * If it is an error event, then a user-specified routine is
 * called if it was defined, otherwise we clean up and exit.
 */
int GrGetNextEvent(GR_EVENT *ep)
{
	char 	c;
	fd_set 	rfds;
	int	setsize = 0;

	if(regfd != -1) {
		c = GrNumGetNextEvent;
		write(sock, &c, 1); /* fixme: check return code*/
		FD_ZERO(&rfds);
		FD_SET(sock, &rfds);
		FD_SET(regfd, &rfds);
		if(sock > setsize) setsize = sock;
		if(regfd > setsize) setsize = regfd;
		++setsize;
		if(select(setsize, &rfds, NULL, NULL, NULL) > 0) {
			if(FD_ISSET(sock, &rfds)) {
				/* fixme: check return code*/
				read(sock, &c, 1);
				if(c != GrRetDataFollows)
					return -1;
				goto readevent;
			}
			if(FD_ISSET(regfd, &rfds)) {
				ep->type = GR_EVENT_TYPE_FDINPUT;
			}
		}
	} else {
		/* send a byte requesting an event check,
		 * wait till event exists
		 */
		if(GrSendByte(GrNumGetNextEvent) != GrRetDataFollows)
			return -1;

readevent:
		/* this will never be GR_EVENT_IDLE
		 * with current implementation
		 */
		if(GrReadBlock(ep, sizeof(*ep)) == -1)
			return -1;
	}
	return 0;
}

/*
 * Return the next event from the event queue if one is ready.
 * If one is not ready, then the type GR_EVENT_TYPE_NONE is returned.
 * If it is an error event, then a user-specified routine is called
 * if it was defined, otherwise we clean up and exit.
 */
int GrCheckNextEvent(GR_EVENT *ep)
{
	if(GrSendByte(GrNumCheckNextEvent) != GrRetDataFollows)
		return -1;

	if(GrReadBlock(ep, sizeof(*ep)) == -1)
		return -1;

	return 0;
}

/*
 * Return the next event from the event queue if available.
 * If there is no event, then a null event type is returned.
 */
int GrPeekEvent(GR_EVENT *ep)
{
	if(GrSendByte(GrNumPeekEvent) != GrRetDataFollows)
		return -1;

	if(GrReadBlock(ep, sizeof(*ep)) == -1)
		return -1;

	return GrReadByte();
}

/*
 * Select events for a window for this client.
 * The events are a bitmask for the events desired.
 */
int GrSelectEvents(GR_WINDOW_ID wid, GR_EVENT_MASK eventmask)
{
	if(GrSendByte(GrNumSelectEvents) != GrRetSendData)
		return -1;

	if(GrSendBlock(&wid, sizeof(wid)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&eventmask, sizeof(eventmask)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Allocate a new window which is a child of the specified window.
 * The window inherits the cursor of the parent window.
 */
GR_WINDOW_ID GrNewWindow(GR_WINDOW_ID parent, GR_COORD x, GR_COORD y, GR_SIZE width,
			GR_SIZE height, GR_SIZE bordersize, GR_COLOR background,
			GR_COLOR bordercolor)
{
	GR_WINDOW_ID wid;

	if(GrSendByte(GrNumNewWindow) != GrRetSendData)
		return -1;

	if(GrSendBlock(&parent, sizeof(parent)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&x, sizeof(x)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&y, sizeof(y)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&width, sizeof(width)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&height, sizeof(height)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&bordersize, sizeof(bordersize)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&background, sizeof(background)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&bordercolor, sizeof(bordercolor)) != GrRetDataFollows)
		return -1;

	if(GrReadBlock(&wid, sizeof(wid)) == -1)
		return -1;

	return wid;
}

/*
 * Allocate a new input-only window which is a child of the specified window.
 * The window inherits the cursor of the parent window.
 */
GR_WINDOW_ID GrNewInputWindow(GR_WINDOW_ID parent, GR_COORD x, GR_COORD y, GR_SIZE width,
			GR_SIZE height)
{
	GR_WINDOW_ID wid;

	if(GrSendByte(GrNumNewInputWindow) != GrRetSendData)
		return -1;

	if(GrSendBlock(&parent, sizeof(parent)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&x, sizeof(x)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&y, sizeof(y)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&width, sizeof(width)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&height, sizeof(height)) != GrRetSendData)
		return -1;

	if(GrReadBlock(&wid, sizeof(wid)) == -1)
		return -1;

	return wid;
}

/*
 * Destroy an existing window.
 */
int GrDestroyWindow(GR_WINDOW_ID wid)
{
	if(GrSendByte(GrNumDestroyWindow) != GrRetSendData)
		return -1;

	if(GrSendBlock(&wid, sizeof(wid)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Return information about a window id.
 */
int GrGetWindowInfo(GR_WINDOW_ID wid, GR_WINDOW_INFO *infoptr)
{
	if(GrSendByte(GrNumGetWindowInfo) != GrRetSendData)
		return -1;

	if(GrSendBlock(&wid, sizeof(wid)) != GrRetDataFollows)
		return -1;

	if(GrReadBlock(infoptr, sizeof(GR_WINDOW_INFO)) == -1)
		return -1;

	return 0;
}

/*
 * Allocate a new GC with default parameters.
 */
GR_GC_ID GrNewGC(void)
{
	GR_GC_ID gc;

	if(GrSendByte(GrNumNewGC) != GrRetDataFollows)
		return -1;

	if(GrReadBlock(&gc, sizeof(gc)) == -1)
		return -1;

	return gc;
}

/*
 * Allocate a new GC which is a copy of another one.
 */
GR_GC_ID GrCopyGC(GR_GC_ID gc)
{
	GR_GC_ID newgc;

	if(GrSendByte(GrNumCopyGC) != GrRetSendData)
		return -1;

	if(GrSendBlock(&gc, sizeof(gc)) != GrRetDataFollows)
		return -1;

	if(GrReadBlock(&newgc, sizeof(newgc)) == -1)
		return -1;

	return newgc;
}

/*
 * Destroy an existing graphics context.
 */
int GrDestroyGC(GR_GC_ID gc)
{
	if(GrSendByte(GrNumDestroyGC) != GrRetSendData)
		return -1;

	if(GrSendBlock(&gc, sizeof(gc)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Map the window to make it (and possibly its children) visible on the screen.
 * This paints the border and background of the window, and creates an
 * exposure event to tell the client to draw into it.
 */
int GrMapWindow(GR_WINDOW_ID wid)
{
	if(GrSendByte(GrNumMapWindow) != GrRetSendData)
		return -1;

	if(GrSendBlock(&wid, sizeof(wid)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Unmap the window to make it and its children invisible on the screen.
 */
int GrUnmapWindow(GR_WINDOW_ID wid)
{
	if(GrSendByte(GrNumUnmapWindow) != GrRetSendData)
		return -1;

	if(GrSendBlock(&wid, sizeof(wid)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Raise the window to the highest level among its siblings.
 */
int GrRaiseWindow(GR_WINDOW_ID wid)
{
	if(GrSendByte(GrNumRaiseWindow) != GrRetSendData)
		return -1;

	if(GrSendBlock(&wid, sizeof(wid)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Lower the window to the lowest level among its siblings.
 */
int GrLowerWindow(GR_WINDOW_ID wid)
{
	if(GrSendByte(GrNumLowerWindow) != GrRetSendData)
		return -1;

	if(GrSendBlock(&wid, sizeof(wid)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Move the window to the specified position relative to its parent.
 */
int GrMoveWindow(GR_WINDOW_ID wid, GR_COORD x, GR_COORD y)
{
	if(GrSendByte(GrNumMoveWindow) != GrRetSendData)
		return -1;

	if(GrSendBlock(&wid, sizeof(wid)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&x, sizeof(x)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&y, sizeof(y)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Resize the window to be the specified size.
 */
int GrResizeWindow(GR_WINDOW_ID wid, GR_SIZE width, GR_SIZE height)
{
	if(GrSendByte(GrNumResizeWindow) != GrRetSendData)
		return -1;

	if(GrSendBlock(&wid, sizeof(wid)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&width, sizeof(width)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&height, sizeof(height)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Clear the specified window by setting it to its background color.
 * If the exposeflag is nonzero, then this also creates an exposure
 * event for the window.
 */
int GrClearWindow(GR_WINDOW_ID wid, GR_BOOL exposeflag)
{
	if(GrSendByte(GrNumClearWindow) != GrRetSendData)
		return -1;

	if(GrSendBlock(&wid, sizeof(wid)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&exposeflag, sizeof(exposeflag)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Set the focus to a particular window.
 * This makes keyboard events only visible to that window or children of it,
 * depending on the pointer location.
 */
int GrSetFocus(GR_WINDOW_ID wid)
{
	if(GrSendByte(GrNumSetFocus) != GrRetSendData)
		return -1;

	if(GrSendBlock(&wid, sizeof(wid)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Set the border of a window to the specified color.
 */
int GrSetBorderColor(GR_WINDOW_ID wid, GR_COLOR colour)
{
	if(GrSendByte(GrNumSetBorderColor) != GrRetSendData)
		return -1;

	if(GrSendBlock(&wid, sizeof(wid)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&colour, sizeof(colour)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Specify a cursor for a window.
 * This cursor will only be used within that window, and by default
 * for its new children.  If the cursor is currently within this
 * window, it will be changed to the new one immediately.
 */
int GrSetCursor(GR_WINDOW_ID wid, GR_SIZE width, GR_SIZE height, GR_COORD hotx,
		GR_COORD hoty, GR_COLOR foreground, GR_COLOR background,
		GR_BITMAP *fgbitmap, GR_BITMAP *bgbitmap)
{
	int bitmapsize = GR_BITMAP_SIZE(width, height) * sizeof(GR_BITMAP);
 
	if(GrSendByte(GrNumSetCursor) != GrRetSendData)
		return -1;

	if(GrSendBlock(&wid, sizeof(wid)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&width, sizeof(width)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&height, sizeof(height)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&hotx, sizeof(hotx)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&hoty, sizeof(hoty)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&foreground, sizeof(foreground)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&background, sizeof(background)) != GrRetSendData)
		return -1;

	if(GrSendBlock(fgbitmap, bitmapsize) != GrRetSendData)
		return -1;

	if(GrSendBlock(bgbitmap, bitmapsize) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Move the cursor to the specified absolute screen coordinates.
 * The coordinates are that of the defined hot spot of the cursor.
 * The cursor's appearance is changed to that defined for the window
 * in which the cursor is moved to.
 */
int GrMoveCursor(GR_COORD x, GR_COORD y)
{
	if(GrSendByte(GrNumMoveCursor) != GrRetSendData)
		return -1;

	if(GrSendBlock(&x, sizeof(x)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&y, sizeof(y)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Flush the message buffer of any messages it may contain.
 */
int GrFlush(void)
{
	if(GrSendByte(GrNumFlush) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Set the foreground color in a graphics context.
 */
int GrSetGCForeground(GR_GC_ID gc, GR_COLOR foreground)
{
	if(GrSendByte(GrNumSetGCForeground) != GrRetSendData)
		return -1;

	if(GrSendBlock(&gc, sizeof(gc)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&foreground, sizeof(foreground)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Set the background color in a graphics context.
 */
int GrSetGCBackground(GR_GC_ID gc, GR_COLOR background)
{
	if(GrSendByte(GrNumSetGCBackground) != GrRetSendData)
		return -1;

	if(GrSendBlock(&gc, sizeof(gc)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&background, sizeof(background)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Set the drawing mode in a graphics context.
 */
int GrSetGCMode(GR_GC_ID gc, GR_MODE mode)
{
	if(GrSendByte(GrNumSetGCMode) != GrRetSendData)
		return -1;

	if(GrSendBlock(&gc, sizeof(gc)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&mode, sizeof(mode)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Set whether or not the background color is drawn in bitmaps and text.
 */
int GrSetGCUseBackground(GR_GC_ID gc, GR_BOOL flag)
{
	if(GrSendByte(GrNumSetGCUseBackground) != GrRetSendData)
		return -1;

	if(GrSendBlock(&gc, sizeof(gc)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&flag, sizeof(flag)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Set the font used for text drawing in a graphics context.
 * The font is a number identifying one of several fonts.
 * Font number 0 is always available, and is the default font.
 */
int GrSetGCFont(GR_GC_ID gc, GR_FONT font)
{
	if(GrSendByte(GrNumSetGCFont) != GrRetSendData)
		return -1;

	if(GrSendBlock(&gc, sizeof(gc)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&font, sizeof(font)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Draw a line in the specified drawable using the specified graphics context.
 */
int GrLine(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x1, GR_COORD y1, GR_COORD x2, GR_COORD y2)
{
	if(GrSendByte(GrNumLine) != GrRetSendData)
		return -1;

	if(GrSendBlock(&id, sizeof(id)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&gc, sizeof(gc)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&x1, sizeof(x1)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&y1, sizeof(y1)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&x2, sizeof(x2)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&y2, sizeof(y2)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Draw the boundary of a rectangle in the specified drawable using the
 * specified graphics context.
 */
int GrRect(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE width, GR_SIZE height)
{
	if(GrSendByte(GrNumRect) != GrRetSendData)
		return -1;

	if(GrSendBlock(&id, sizeof(id)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&gc, sizeof(gc)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&x, sizeof(x)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&y, sizeof(y)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&width, sizeof(width)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&height, sizeof(height)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Fill a rectangle in the specified drawable using the specified
 * graphics context.
 */
int GrFillRect(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE width, GR_SIZE height)
{
	if(GrSendByte(GrNumFillRect) != GrRetSendData)
		return -1;

	if(GrSendBlock(&id, sizeof(id)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&gc, sizeof(gc)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&x, sizeof(x)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&y, sizeof(y)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&width, sizeof(width)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&height, sizeof(height)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Draw the boundary of an ellipse in the specified drawable with
 * the specified graphics context.
 */
int GrEllipse(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE rx, GR_SIZE ry)
{
	if(GrSendByte(GrNumEllipse) != GrRetSendData)
		return -1;

	if(GrSendBlock(&id, sizeof(id)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&gc, sizeof(gc)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&x, sizeof(x)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&y, sizeof(y)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&rx, sizeof(rx)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&ry, sizeof(ry)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Fill an ellipse in the specified drawable using the specified
 * graphics context.
 */
int GrFillEllipse(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE rx, GR_SIZE ry)
{
	if(GrSendByte(GrNumFillEllipse) != GrRetSendData)
		return -1;

	if(GrSendBlock(&id, sizeof(id)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&gc, sizeof(gc)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&x, sizeof(x)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&y, sizeof(y)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&rx, sizeof(rx)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&ry, sizeof(ry)) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Draw a rectangular area in the specified drawable using the specified
 * graphics, as determined by the specified bit map.  This differs from
 * rectangle drawing in that the rectangle is drawn using the foreground
 * color and possibly the background color as determined by the bit map.
 * Each row of bits is aligned to the next bitmap word boundary (so there
 * is padding at the end of the row).  The background bit values are
 * only written if the usebackground flag is set in the GC.
 */
int GrBitmap(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE width,
	GR_SIZE height, GR_BITMAP *bitmaptable)
{
	long bitmapsize = (long)GR_BITMAP_SIZE(width, height) * sizeof(GR_BITMAP);

	if(GrSendByte(GrNumBitmap) != GrRetSendData)
		return -1;

	if(GrSendBlock(&id, sizeof(id)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&gc, sizeof(gc)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&x, sizeof(x)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&y, sizeof(y)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&width, sizeof(width)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&height, sizeof(height)) != GrRetSendData)
		return -1;

	if(GrSendBlock(bitmaptable, bitmapsize) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Draw a rectangular area in the specified drawable using the specified
 * graphics context.  This differs from rectangle drawing in that the
 * color values for each pixel in the rectangle are specified.
 * The color table is indexed
 * row by row.  Values whose color matches the background color are only
 * written if the usebackground flag is set in the GC.
 */
int GrArea(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE width,
	GR_SIZE height, PIXELVAL *pixels)
{
	/* FIXME: optimize for smaller pixelvals*/
	long size = (long)width * height * sizeof(PIXELVAL);

	if(GrSendByte(GrNumArea) != GrRetSendData)
		return -1;

	if(GrSendBlock(&id, sizeof(id)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&gc, sizeof(gc)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&x, sizeof(x)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&y, sizeof(y)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&width, sizeof(width)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&height, sizeof(height)) != GrRetSendData)
		return -1;

	if(GrSendBlock(pixels, size) != GrRetOK)
		return -1;
	return 0;
}

/*
 * Read the color values from the specified rectangular area of the
 * specified drawable into a supplied buffer.  If the drawable is a
 * window which is obscured by other windows, then the returned values
 * will include the values from the covering windows.  Regions outside
 * of the screen boundaries, or unmapped windows will return black.
 */
int GrReadArea(GR_DRAW_ID id,GR_COORD x,GR_COORD y,GR_SIZE width,GR_SIZE height,
	PIXELVAL *pixels)
{
	/* FIXME: optimize for smaller pixelvals*/
	long size = (long)width * height * sizeof(PIXELVAL);

	if(GrSendByte(GrNumReadArea) != GrRetSendData)
		return -1;

	if(GrSendBlock(&id, sizeof(id)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&x, sizeof(x)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&y, sizeof(y)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&width, sizeof(width)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&height, sizeof(height)) != GrRetDataFollows)
		return -1;

	if(GrReadBlock(pixels, size) == -1)
		return -1;
	return 0;
}

/*
 * Draw a point in the specified drawable using the specified
 * graphics context.
 */
int GrPoint(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y)
{
	if(GrSendByte(GrNumPoint) != GrRetSendData)
		return -1;

	if(GrSendBlock(&id, sizeof(id)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&gc, sizeof(gc)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&x, sizeof(x)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&y, sizeof(y)) != GrRetOK)
		return -1;

	return 0;

}

/*
 * Draw a polygon in the specified drawable using the specified
 * graphics context.  The polygon is only complete if the first
 * point is repeated at the end.
 */
int GrPoly(GR_DRAW_ID id, GR_GC_ID gc, GR_COUNT count, GR_POINT *pointtable)
{
	if(GrSendByte(GrNumPoly) != GrRetSendData)
		return -1;

	if(GrSendBlock(&id, sizeof(id)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&gc, sizeof(gc)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&count, sizeof(count)) != GrRetSendData)
		return -1;

	if(GrSendBlock(pointtable, (count * sizeof(GR_POINT))) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Draw a filled polygon in the specified drawable using the specified
 * graphics context.  The last point may be a duplicate of the first
 * point, but this is not required.
 */
int GrFillPoly(GR_DRAW_ID id, GR_GC_ID gc, GR_COUNT count, GR_POINT *pointtable)
{
	if(GrSendByte(GrNumFillPoly) != GrRetSendData)
		return -1;

	if(GrSendBlock(&id, sizeof(id)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&gc, sizeof(gc)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&count, sizeof(count)) != GrRetSendData)
		return -1;

	if(GrSendBlock(pointtable, (count * sizeof(GR_POINT))) != GrRetOK)
		return -1;

	return 0;
}

/*
 * Draw a text string in the specified drawable using the specified
 * graphics context.  The background of the characters are only drawn
 * if the usebackground flag in the GC is set.
 */
int GrText(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_CHAR *str, GR_COUNT count)
{
	if(GrSendByte(GrNumText) != GrRetSendData)
		return -1;

	if(GrSendBlock(&id, sizeof(id)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&gc, sizeof(gc)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&x, sizeof(x)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&y, sizeof(y)) != GrRetSendData)
		return -1;

	if(GrSendBlock(&count, sizeof(count)) != GrRetSendData)
		return -1;

	if(GrSendBlock(str, count) != GrRetOK)
		return -1;

	return 0;
}
