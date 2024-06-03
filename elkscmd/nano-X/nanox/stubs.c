/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Stub routines for direct client linking with nanoX
 */
#include <stdio.h>
#include "serv.h"

/*
 * Open a connection to the graphics server.
 * Returns the fd of the connection to the server if successful, or -1 on failure.
 */
int GrOpen(void)
{
	return GsOpen();
}

/*
 * Close the graphics device, first flushing any waiting messages.
 */
int GrClose(void)
{
	GsClose();
	return 0;
}

/*
 * Set an error handling routine, which will be called on any errors from
 * the server (when events are asked for by the client).  If zero is given,
 * then a default routine will be used.  Returns the previous error handler.
 */
GR_ERROR_FUNC GrSetErrorHandler(GR_ERROR_FUNC func)
{
#if 0
	GR_ERROR_FUNC temp = GrErrorFunc;
	if(!func) GrErrorFunc = &GrDefaultErrorHandler;
	else GrErrorFunc = func;
	return temp;
#endif
	return 0;
}

/*
 * Return useful information about the screen.
 * Returns -1 on failure or 0 on success.
 */
int GrGetScreenInfo(GR_SCREEN_INFO *sip)
{
	GsGetScreenInfo(sip);
	return 0;
}

/*
 * Return useful information about the specified font.
 */
int GrGetFontInfo(GR_FONT font, GR_FONT_INFO *fip)
{
	GsGetFontInfo(font, fip);
	return 0;
}

/*
 * Return information about the specified graphics context.
 */
int GrGetGCInfo(GR_GC_ID gc, GR_GC_INFO *gcip)
{
	GsGetGCInfo(gc, gcip);
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
	GsGetGCTextSize(gc, cp, len, retwidth, retheight, retbase);
	return 0;
}

#if UNIX
/*
 * Register the specified file descriptor to return an event
 * when input is ready.
 * FIXME: only one external file descriptor works
 */
int GrRegisterInput(int fd)
{
	GsRegisterInput(fd);
	return 0;
}
#endif

/*
 * Return the next event from the event queue.
 * This waits for a new one if one is not ready.
 * If it is an error event, then a user-specified routine is
 * called if it was defined, otherwise we clean up and exit.
 */
int GrGetNextEvent(GR_EVENT *ep)
{
	GsCheckNextEvent(ep, GR_TIMEOUT_BLOCK);
	return 0;
}

int GrGetNextEventTimeout(GR_EVENT *ep, GR_TIMEOUT timeout)
{
    GsCheckNextEvent(ep, timeout);
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
	GsCheckNextEvent(ep, GR_TIMEOUT_POLL);
	return 0;
}

/*
 * Return the next event from the event queue if available.
 * If there is no event, then a null event type is returned.
 */
int GrPeekEvent(GR_EVENT *ep)
{
	return GsPeekEvent(ep);
}

/*
 * Select events for a window for this client.
 * The events are a bitmask for the events desired.
 */
int GrSelectEvents(GR_WINDOW_ID wid, GR_EVENT_MASK eventmask)
{
	GsSelectEvents(wid, eventmask);
	return 0;
}

/*
 * Allocate a new window which is a child of the specified window.
 * The window inherits the cursor of the parent window.
 */
GR_WINDOW_ID GrNewWindow(GR_WINDOW_ID parent, GR_COORD x, GR_COORD y,
		GR_SIZE width, GR_SIZE height, GR_SIZE bordersize,
		GR_COLOR background, GR_COLOR bordercolor)
{
	return GsNewWindow(parent, x, y, width, height, bordersize,
		background, bordercolor);
}

/*
 * Allocate a new input-only window which is a child of the specified window.
 * The window inherits the cursor of the parent window.
 */
GR_WINDOW_ID GrNewInputWindow(GR_WINDOW_ID parent, GR_COORD x, GR_COORD y, GR_SIZE width,
			GR_SIZE height)
{
	return GsNewInputWindow(parent, x, y, width, height);
}

/*
 * Destroy an existing window.
 */
int GrDestroyWindow(GR_WINDOW_ID wid)
{
	GsDestroyWindow(wid);
	return 0;
}

/*
 * Return information about a window id.
 */
int GrGetWindowInfo(GR_WINDOW_ID wid, GR_WINDOW_INFO *infoptr)
{
	GsGetWindowInfo(wid, infoptr);
	return 0;
}

/*
 * Allocate a new GC with default parameters.
 */
GR_GC_ID GrNewGC(void)
{
	return GsNewGC();
}

/*
 * Allocate a new GC which is a copy of another one.
 */
GR_GC_ID GrCopyGC(GR_GC_ID gc)
{
	return GsCopyGC(gc);
}

/*
 * Destroy an existing graphics context.
 */
int GrDestroyGC(GR_GC_ID gc)
{
	GsDestroyGC(gc);
	return 0;
}

/*
 * Map the window to make it (and possibly its children) visible on the screen.
 * This paints the border and background of the window, and creates an
 * exposure event to tell the client to draw into it.
 */
int GrMapWindow(GR_WINDOW_ID wid)
{
	GsMapWindow(wid);
	return 0;
}

/*
 * Unmap the window to make it and its children invisible on the screen.
 */
int GrUnmapWindow(GR_WINDOW_ID wid)
{
	GsUnmapWindow(wid);
	return 0;
}

/*
 * Raise the window to the highest level among its siblings.
 */
int GrRaiseWindow(GR_WINDOW_ID wid)
{
	GsRaiseWindow(wid);
	return 0;
}

/*
 * Lower the window to the lowest level among its siblings.
 */
int GrLowerWindow(GR_WINDOW_ID wid)
{
	GsLowerWindow(wid);
	return 0;
}

/*
 * Move the window to the specified position relative to its parent.
 */
int GrMoveWindow(GR_WINDOW_ID wid, GR_COORD x, GR_COORD y)
{
	GsMoveWindow(wid, x, y);
	return 0;
}

/*
 * Resize the window to be the specified size.
 */
int GrResizeWindow(GR_WINDOW_ID wid, GR_SIZE width, GR_SIZE height)
{
	GsResizeWindow(wid, width, height);
	return 0;
}

/*
 * Clear the specified window by setting it to its background color.
 * If the exposeflag is nonzero, then this also creates an exposure
 * event for the window.
 */
int GrClearWindow(GR_WINDOW_ID wid, GR_BOOL exposeflag)
{
	GsClearWindow(wid, exposeflag);
	return 0;
}

/*
 * Set the focus to a particular window.
 * This makes keyboard events only visible to that window or children of it,
 * depending on the pointer location.
 */
int GrSetFocus(GR_WINDOW_ID wid)
{
	GsSetFocus(wid);
	return 0;
}

/*
 * Set the border of a window to the specified color.
 */
int GrSetBorderColor(GR_WINDOW_ID wid, GR_COLOR colour)
{
	GsSetBorderColor(wid, colour);
	return 0;
}

/*
 * Specify a cursor for a window.
 * This cursor will only be used within that window, and by default
 * for its new children.  If the cursor is currently within this
 * window, it will be changed to the new one immediately.
 */
int GrSetCursor(GR_WINDOW_ID wid, GR_SIZE width, GR_SIZE height,
	GR_COORD hotx, GR_COORD hoty, GR_COLOR foreground,GR_COLOR background,
	GR_BITMAP *fgbitmap, GR_BITMAP *bgbitmap)
{
	GsSetCursor(wid, width, height, hotx, hoty, foreground,
		background, fgbitmap, bgbitmap);
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
	GsMoveCursor(x, y);
	return 0;
}

/*
 * Flush the message buffer of any messages it may contain.
 */
int GrFlush(void)
{
	return 0;
}

/*
 * Set the foreground color in a graphics context.
 */
int GrSetGCForeground(GR_GC_ID gc, GR_COLOR foreground)
{
	GsSetGCForeground(gc, foreground);
	return 0;
}

/*
 * Set the background color in a graphics context.
 */
int GrSetGCBackground(GR_GC_ID gc, GR_COLOR background)
{
	GsSetGCBackground(gc, background);
	return 0;
}

/*
 * Set the drawing mode in a graphics context.
 */
int GrSetGCMode(GR_GC_ID gc, GR_MODE mode)
{
	GsSetGCMode(gc, mode);
	return 0;
}

/*
 * Set whether or not the background color is drawn in bitmaps and text.
 */
int GrSetGCUseBackground(GR_GC_ID gc, GR_BOOL flag)
{
	GsSetGCUseBackground(gc, flag);
	return 0;
}

/*
 * Set the font used for text drawing in a graphics context.
 * The font is a number identifying one of several fonts.
 * Font number 0 is always available, and is the default font.
 */
int GrSetGCFont(GR_GC_ID gc, GR_FONT font)
{
	GsSetGCFont(gc, font);
	return 0;
}

/*
 * Draw a line in the specified drawable using the specified graphics context.
 */
int GrLine(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x1, GR_COORD y1, GR_COORD x2, GR_COORD y2)
{
	GsLine(id, gc, x1, y1, x2, y2);
	return 0;
}

/*
 * Draw the boundary of a rectangle in the specified drawable using the
 * specified graphics context.
 */
int GrRect(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE width, GR_SIZE height)
{
	GsRect(id, gc, x, y, width, height);
	return 0;
}

/*
 * Fill a rectangle in the specified drawable using the specified
 * graphics context.
 */
int GrFillRect(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE width, GR_SIZE height)
{
	GsFillRect(id, gc, x, y, width, height);
	return 0;
}

/*
 * Draw the boundary of an ellipse in the specified drawable with
 * the specified graphics context.
 */
int GrEllipse(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE rx, GR_SIZE ry)
{
	GsEllipse(id, gc, x, y, rx, ry);
	return 0;
}

/*
 * Fill an ellipse in the specified drawable using the specified
 * graphics context.
 */
int GrFillEllipse(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE rx, GR_SIZE ry)
{
	GsFillEllipse(id, gc, x, y, rx, ry);
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
int GrBitmap(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE width, GR_SIZE height,
		GR_BITMAP *bitmaptable)
{
	GsBitmap(id, gc, x, y, width, height, bitmaptable);
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
int GrArea(GR_DRAW_ID id,GR_GC_ID gc,GR_COORD x,GR_COORD y,GR_SIZE width,
	GR_SIZE height, PIXELVAL *pixels)
{
	GsArea(id, gc, x, y, width, height, pixels);
	return 0;
}

/*
 * Read the color values from the specified rectangular area of the
 * specified drawable into a supplied buffer.  If the drawable is a
 * window which is obscured by other windows, then the returned values
 * will include the values from the covering windows.  Regions outside
 * of the screen boundaries, or unmapped windows will return black.
 */
int GrReadArea(GR_DRAW_ID id, GR_COORD x, GR_COORD y, GR_SIZE width, 
	GR_SIZE height, PIXELVAL *pixels)
{
	GsReadArea(id, x, y, width, height, pixels);
	return 0;
}

/*
 * Draw a point in the specified drawable using the specified
 * graphics context.
 */
int GrPoint(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y)
{
	GsPoint(id, gc, x, y);
	return 0;

}

/*
 * Draw a polygon in the specified drawable using the specified
 * graphics context.  The polygon is only complete if the first
 * point is repeated at the end.
 */
int GrPoly(GR_DRAW_ID id, GR_GC_ID gc, GR_COUNT count, GR_POINT *pointtable)
{
	GsPoly(id, gc, count, pointtable);
	return 0;
}

/*
 * Draw a filled polygon in the specified drawable using the specified
 * graphics context.  The last point may be a duplicate of the first
 * point, but this is not required.
 */
int GrFillPoly(GR_DRAW_ID id, GR_GC_ID gc, GR_COUNT count, GR_POINT *pointtable)
{
	GsFillPoly(id, gc, count, pointtable);
	return 0;
}

/*
 * Draw a text string in the specified drawable using the specified
 * graphics context.  The background of the characters are only drawn
 * if the usebackground flag in the GC is set.
 */
int GrText(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_CHAR *str, GR_COUNT count)
{
	GsText(id, gc, x, y, str, count);
	return 0;
}

#if 0000
/*
 * The following is the user defined function for handling errors.
 * If this is not set, then the default action is to close the connection
 * to the server, describe the error, and then exit.  This error function
 * will only be called when the client asks for events.
 */
static GR_ERROR_FUNC GrErrorFunc = &GrDefaultErrorHandler;


/*
 * Read an error event structure from the server and deliver it to the client.
 */
static int GrDeliverErrorEvent(void)
{
	GR_EVENT_ERROR err;

	GrErrorFunc(err);

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
#endif
