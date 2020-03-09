/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serv.h"

PSD psd = &scrdev;	/* temporary*/

/*
 * Redraw the screen completely.
 */
void
GsRedrawScreen(void)
{
	GR_SCREEN_INFO	sinfo;

	GdGetScreenInfo(psd, &sinfo);

	/* Redraw all windows*/
	/* FIXME: doesn't redraw root window border*/
	GsExposeArea(rootwp, 0, 0, sinfo.cols, sinfo.rows);
}

/*
 * Flush graphics to the device.
 */
void GsFlush(void)
{
}

/*
 * Return information about the screen for clients to use.
 */
void GsGetScreenInfo(GR_SCREEN_INFO *sip)
{
	/* FIXME after dynamic font loads, must call driver again*/
	*sip = sinfo;
}

/*
 * Return the size of a text string for the font in a graphics context.
 * This is the width of the string, the height of the string,
 * and the height above the bottom of the font of the baseline for the font.
 */
void GsGetGCTextSize(GR_GC_ID gc, GR_CHAR *cp, GR_SIZE len, GR_SIZE *retwidth,
			GR_SIZE *retheight, GR_SIZE *retbase)
{
	GR_GC		*gcp;		/* graphics context */
	GR_FONT_INFO	*fip;		/* current font info */

	gcp = GsFindGC(gc);
	if (gcp == NULL) {
		*retwidth = *retbase = 0;
		*retheight = 1;
		return;
	}

	fip = &curfont;
	if (fip->font != gcp->font)
		GdGetFontInfo(psd, gcp->font, fip);

	*retheight = fip->height;
	*retbase = fip->baseline;
	if (fip->fixed) {
		*retwidth = fip->maxwidth * len;
		return;
	}

	*retwidth = 0;
	while (len-- > 0)
		*retwidth += fip->widths[*cp++];
}

#if NONETWORK
/*
 * Return the next waiting event for a client, or wait for one if there
 * is none yet.  The event is copied into the specified structure, and
 * then is moved from the event queue to the free event queue.  If there
 * is an error event waiting, it is delivered before any other events.
 */
void GsGetNextEvent(GR_EVENT *ep)
{
	/* If no event ready, wait for one*/
	/* Note: won't work for multiple clients*/
	/* This is OK, since only static linked apps call this function*/
	while(curclient->eventhead == NULL)
		GsSelect();
	GsCheckNextEvent(ep);
}
#endif

/*
 * Return the next event from the event queue if one is ready.
 * If one is not ready, then the type GR_EVENT_TYPE_NONE is returned.
 * If it is an error event, then a user-specified routine is called
 * if it was defined, otherwise we clean up and exit.
 */
void GsCheckNextEvent(GR_EVENT *ep)
{
	GR_EVENT_LIST *	elp;

#if NONETWORK
	/* Since we're bound to server, select() is only called
	 * thru here
	 */
	GsSelect();
#endif
	/* Copy first event if any*/
	if(!GsPeekEvent(ep))
		return;

	/* Get first event again*/
	elp = curclient->eventhead;

	/* Remove first event from queue*/
	curclient->eventhead = elp->next;
	if (curclient->eventtail == elp)
		curclient->eventtail = NULL;

	elp->next = eventfree;
	eventfree = elp;
}

/*
 * Peek at the event queue for the current client to see if there are any
 * outstanding events.  Returns the event at the head of the queue, or
 * else a null event type.  The event is still left in the queue, however.
 */
int GsPeekEvent(GR_EVENT *ep)
{
	GR_EVENT_LIST *	elp;

	elp = curclient->eventhead;
	if(elp == NULL) {
		ep->type = GR_EVENT_TYPE_NONE;
		return 0;
	}

	/* copy event out*/
	*ep = elp->event;
	return 1;
}

/*
 * Return information about a window id.
 */
void GsGetWindowInfo(GR_WINDOW_ID wid, GR_WINDOW_INFO *infoptr)
{
	GR_WINDOW	*wp;		/* window structure */
	GR_EVENT_CLIENT	*evp;		/* event-client structure */

	/*
	 * Find the window manually so that an error is not generated.
	 */
	for (wp = listwp; wp && (wp->id != wid); wp = wp->next)
		continue;

	if (wp == NULL) {
		infoptr->wid = 0;
		return;
	}

	infoptr->wid = wid;
	infoptr->parent = wp->parent->id;
	infoptr->child = wp->children->id;
	infoptr->sibling = wp->siblings->id;
	infoptr->mapped = wp->mapped;
	infoptr->unmapcount = wp->unmapcount;
	infoptr->inputonly = !wp->output;
	infoptr->x = wp->x;
	infoptr->y = wp->y;
	infoptr->width = wp->width;
	infoptr->height = wp->height;
	infoptr->bordersize = wp->bordersize;
	infoptr->bordercolor = wp->bordercolor;
	infoptr->background = wp->background;
	infoptr->eventmask = 0;

	for (evp = wp->eventclients; evp; evp = evp->next) {
		if (evp->client == curclient)
			infoptr->eventmask = evp->eventmask;
	}
}

/*
 * Destroy an existing window and all of its children.
 */
void GsDestroyWindow(GR_WINDOW_ID wid)
{
	GR_WINDOW	*wp;		/* window structure */

	wp = GsFindWindow(wid);
	if (wp)
		GsWpDestroyWindow(wp);
}


/*
 * Raise a window to the highest level among its siblings.
 */
void GsRaiseWindow(GR_WINDOW_ID wid)
{
	GR_WINDOW	*wp;		/* window structure */
	GR_WINDOW	*prevwp;	/* previous window pointer */
	GR_BOOL		overlap;	/* TRUE if there was overlap */

	wp = GsFindWindow(wid);
	if ((wp == NULL) || (wp == rootwp))
		return;

	/*
	 * If this is already the highest window then we are done.
	 */
	prevwp = wp->parent->children;
	if (prevwp == wp)
		return;

	/*
	 * Find the sibling just before this window so we can unlink it.
	 * Also, determine if any sibling ahead of us overlaps the window.
	 * Remember that for exposure events.
	 */
	overlap = GR_FALSE;
	while (prevwp->siblings != wp) {
		overlap |= GsCheckOverlap(prevwp, wp);
		prevwp = prevwp->siblings;
	}
	overlap |= GsCheckOverlap(prevwp, wp);

	/*
	 * Now unlink the window and relink it in at the front of the
	 * sibling chain.
	 */
	prevwp->siblings = wp->siblings;
	wp->siblings = wp->parent->children;
	wp->parent->children = wp;

	/*
	 * Finally redraw the window if necessary.
	 */
	if (overlap) {
		GsDrawBorder(wp);
		GsExposeArea(wp, wp->x, wp->y, wp->width, wp->height);
	}
}

/*
 * Lower a window to the lowest level among its siblings.
 */
void GsLowerWindow(GR_WINDOW_ID wid)
{
	GR_WINDOW	*wp;		/* window structure */
	GR_WINDOW	*prevwp;	/* previous window pointer */
	GR_WINDOW	*sibwp;		/* sibling window */
	GR_WINDOW	*expwp;		/* siblings being exposed */

	wp = GsFindWindow(wid);
	if ((wp == NULL) || (wp == rootwp) || (wp->siblings == NULL))
		return;

	/*
	 * Find the sibling just before this window so we can unlink us.
	 */
	prevwp = wp->parent->children;
	if (prevwp != wp) {
		while (prevwp->siblings != wp)
			prevwp = prevwp->siblings;
	}

	/*
	 * Remember the first sibling that is after us, so we can
	 * generate exposure events for the remaining siblings.  Then
	 * walk down the sibling chain looking for the last sibling.
	 */
	expwp = wp->siblings;
	sibwp = wp;
	while (sibwp->siblings)
		sibwp = sibwp->siblings;

	/*
	 * Now unlink the window and relink it in at the end of the
	 * sibling chain.
	 */
	if (prevwp == wp)
		wp->parent->children = wp->siblings;
	else
		prevwp->siblings = wp->siblings;
	sibwp->siblings = wp;

	wp->siblings = NULL;

	/*
	 * Finally redraw the sibling windows which this window covered
	 * if they overlapped our window.
	 */
	while (expwp && (expwp != wp)) {
		if (GsCheckOverlap(wp, expwp)) {
			GsExposeArea(expwp, wp->x - wp->bordersize,
				wp->y - wp->bordersize,
				wp->width + wp->bordersize * 2,
				wp->height + wp->bordersize * 2);
		}
		expwp = expwp->siblings;
	}
}

/*
 * Move the window to the specified position relative to its parent.
 */
void GsMoveWindow(GR_WINDOW_ID wid, GR_COORD x, GR_COORD y)
{
	GR_WINDOW	*wp;		/* window structure */

	wp = GsFindWindow(wid);
	if (wp == NULL)
		return;
	if (wp == rootwp) {
		GsError(GR_ERROR_ILLEGAL_ON_ROOT_WINDOW, wid);
		return;
	}

	x += wp->parent->x;
	y += wp->parent->y;

	if ((wp->x == x) && (wp->y == y))
		return;

	if (wp->unmapcount || !wp->output) {
		wp->x = x;
		wp->y = y;
		return;
	}

	/*
	 * This should be optimized to not require redrawing of the window
	 * when possible.
	 */
	GsWpUnmapWindow(wp);
	wp->x = x;
	wp->y = y;
	GsWpMapWindow(wp);
}

/*
 * Resize the window to be the specified size.
 */
void GsResizeWindow(GR_WINDOW_ID wid, GR_SIZE width, GR_SIZE height)
{
	GR_WINDOW	*wp;		/* window structure */

	wp = GsFindWindow(wid);
	if (wp == NULL)
		return;
	if (wp == rootwp) {
		GsError(GR_ERROR_ILLEGAL_ON_ROOT_WINDOW, wid);
		return;
	}
	if ((width <= 0) || (height <= 0)) {
		GsError(GR_ERROR_BAD_WINDOW_SIZE, wid);
		return;
	}

	if ((wp->width == width) && (wp->height == height))
		return;

	if (wp->unmapcount || !wp->output) {
		wp->width = width;
		wp->height = height;
		return;
	}

	/*
	 * This should be optimized to not require redrawing of the window
	 * when possible.
	 */
	GsWpUnmapWindow(wp);
	wp->width = width;
	wp->height = height;
	GsWpMapWindow(wp);
}

static int nextgcid = 1000;
/*
 * Allocate a new GC with default parameters.
 * The GC is owned by the current client.
 */
GR_GC_ID GsNewGC(void)
{
	GR_GC	*gcp;

	gcp = (GR_GC *) malloc(sizeof(GR_GC));
	if (gcp == NULL) {
		GsError(GR_ERROR_MALLOC_FAILED, 0);
		return 0;
	}

	gcp->id = nextgcid++;
	gcp->mode = GR_MODE_SET;
	gcp->font = FONT_SYSTEM_VAR;
	gcp->foreground = WHITE;
	gcp->background = BLACK;
	gcp->usebackground = GR_TRUE;
	gcp->changed = GR_TRUE;
	gcp->next = listgcp;

	listgcp = gcp;

	return gcp->id;
}

/*
 * Destroy an existing graphics context.
 */
void GsDestroyGC(GR_GC_ID gc)
{
	GR_GC		*gcp;		/* graphics context */
	GR_GC		*prevgcp;	/* previous graphics context */

	gcp = GsFindGC(gc);
	if (gcp == NULL)
		return;

	if (gc == cachegcid) {
		cachegcid = 0;
		cachegcp = NULL;
	}

	if (listgcp == gcp) {
		listgcp = gcp->next;
		free(gcp);
		return;
	}

	prevgcp = listgcp;
	while (prevgcp->next != gcp)
		prevgcp = prevgcp->next;

	prevgcp->next = gcp->next;
	free(gcp);
}

/*
 * Allocate a new GC which is a copy of another one.
 * The GC is owned by the current client.
 */
GR_GC_ID GsCopyGC(GR_GC_ID gc)
{
	GR_GC		*oldgcp;	/* old graphics context */
	GR_GC		*gcp;		/* new graphics context */

	oldgcp = GsFindGC(gc);
	if (oldgcp == NULL)
		return 0;

	gcp = (GR_GC *) malloc(sizeof(GR_GC));
	if (gcp == NULL) {
		GsError(GR_ERROR_MALLOC_FAILED, 0);
		return 0;
	}

	/*
	 * Copy all the old gcp values into the new one, except allocate
	 * a new id for it and link it into the list of GCs.
	 */
	*gcp = *oldgcp;
	gcp->id = nextgcid++;
	gcp->changed = GR_TRUE;
	gcp->next = listgcp;
	listgcp = gcp;

	return gcp->id;
}

/*
 * Return information about the specified graphics context.
 */
void GsGetGCInfo(GR_GC_ID gcid, GR_GC_INFO *gcip)
{
	GR_GC		*gcp;

	/*
	 * Find the GC manually so that an error is not generated.
	 */
	for (gcp = listgcp; gcp && (gcp->id != gcid); gcp = gcp->next)
		continue;

	if (gcp == NULL) {
		gcip->gcid = 0;
		return;
	}

	gcip->gcid = gcid;
	gcip->mode = gcp->mode;
	gcip->font = gcp->font;
	gcip->foreground = gcp->foreground;
	gcip->background = gcp->background;
	gcip->usebackground = gcp->usebackground;
}

/*
 * Return useful information about the specified font.
 */
void GsGetFontInfo(GR_FONT font, GR_FONT_INFO *fip)
{
	/*
	 * See if the font is built-in or not.  Someday for non-builtin
	 * fonts, we can return something for them.
	 */
	if (font >= sinfo.fonts) {
		fip->font = 0;
		return;
	}
	GdGetFontInfo(psd, font, fip);
}

/*
 * Select events for a window for this client.
 * The events are a bitmask for the events desired.
 */
void GsSelectEvents(GR_WINDOW_ID wid, GR_EVENT_MASK eventmask)
{
	GR_WINDOW	*wp;		/* window structure */
	GR_EVENT_CLIENT	*evp;		/* event-client structure */

	wp = GsFindWindow(wid);
	if (wp == NULL)
		return;

	/*
	 * See if this client is already in the event client list.
	 * If so, then just replace the events he is selecting for.
	 */
	for (evp = wp->eventclients; evp; evp = evp->next) {
		if (evp->client == curclient) {
			evp->eventmask = eventmask;
			return;
		}
	}

	/*
	 * A new client for this window, so allocate a new event client
	 * structure and insert it into the front of the list in the window.
	 */
	evp = (GR_EVENT_CLIENT *) malloc(sizeof(GR_EVENT_CLIENT));
	if (evp == NULL) {
		GsError(GR_ERROR_MALLOC_FAILED, wid);
		return;
	}

	evp->client = curclient;
	evp->eventmask = eventmask;
	evp->next = wp->eventclients;
	wp->eventclients = evp;
}

/*
 * Allocate a new window which is a child of the specified window.
 * The window inherits the cursor of the parent window.
 * The window is owned by the current client.
 */
GR_WINDOW_ID
GsNewWindow(GR_WINDOW_ID parent, GR_COORD x, GR_COORD y, GR_SIZE width,
	GR_SIZE height, GR_SIZE bordersize, GR_COLOR background,
	GR_COLOR bordercolor)
{
	GR_WINDOW	*pwp;		/* parent window */
	GR_WINDOW	*wp;		/* new window */
	static int	nextid = GR_ROOT_WINDOW_ID + 1;

	if ((width <= 0) || (height <= 0) || (bordersize < 0)) {
		GsError(GR_ERROR_BAD_WINDOW_SIZE, 0);
		return 0;
	}

	pwp = GsFindWindow(parent);
	if (pwp == NULL)
		return 0;

	if (!pwp->output) {
		GsError(GR_ERROR_INPUT_ONLY_WINDOW, pwp->id);
		return 0;
	}

	wp = (GR_WINDOW *) malloc(sizeof(GR_WINDOW));
	if (wp == NULL) {
		GsError(GR_ERROR_MALLOC_FAILED, 0);
		return 0;
	}

	wp->id = nextid++;
	wp->parent = pwp;
	wp->children = NULL;
	wp->siblings = pwp->children;
	wp->next = listwp;
	wp->x = pwp->x + x;
	wp->y = pwp->y + y;
	wp->width = width;
	wp->height = height;
	wp->bordersize = bordersize;
	wp->background = background;
	wp->bordercolor = bordercolor;
	wp->nopropmask = 0;
	wp->eventclients = NULL;
	wp->cursor = pwp->cursor;
	wp->cursor->usecount++;
	wp->mapped = GR_FALSE;
	wp->unmapcount = pwp->unmapcount + 1;
	wp->output = GR_TRUE;

	pwp->children = wp;
	listwp = wp;

	return wp->id;
}

/*
 * Allocate a new input-only window which is a child of the specified window.
 * Such a window is invisible, cannot be drawn into, and is only used to
 * return events.  The window inherits the cursor of the parent window.
 * The window is owned by the current client.
 */
GR_WINDOW_ID GsNewInputWindow(GR_WINDOW_ID parent, GR_COORD x, GR_COORD y,
				GR_SIZE width, GR_SIZE height)
{
	GR_WINDOW	*pwp;		/* parent window */
	GR_WINDOW	*wp;		/* new window */

	if ((width <= 0) || (height <= 0)) {
		GsError(GR_ERROR_BAD_WINDOW_SIZE, 0);
		return 0;
	}

	pwp = GsFindWindow(parent);
	if (pwp == NULL)
		return 0;

	wp = (GR_WINDOW *) malloc(sizeof(GR_WINDOW));
	if (wp == NULL) {
		GsError(GR_ERROR_MALLOC_FAILED, 0);
		return 0;
	}

	wp->parent = pwp;
	wp->children = NULL;
	wp->siblings = pwp->children;
	wp->next = listwp;
	wp->x = pwp->x + x;
	wp->y = pwp->y + y;
	wp->width = width;
	wp->height = height;
	wp->bordersize = 0;
	wp->background = BLACK;
	wp->bordercolor = BLACK;
	wp->nopropmask = 0;
	wp->eventclients = NULL;
	wp->cursor = pwp->cursor;
	wp->mapped = GR_FALSE;
	wp->unmapcount = pwp->unmapcount + 1;
	wp->output = GR_FALSE;

	wp->cursor->usecount++;
	pwp->children = wp;
	listwp = wp;

	return wp->id;
}

/*
 * Map the window to make it (and possibly its children) visible on the screen.
 */
void GsMapWindow(GR_WINDOW_ID wid)
{
	GR_WINDOW	*wp;		/* window structure */

	wp = GsFindWindow(wid);
	if ((wp == NULL) || wp->mapped)
		return;

	wp->mapped = GR_TRUE;

	GsWpMapWindow(wp);
}

/*
 * Unmap the window to make it and its children invisible on the screen.
 */
void GsUnmapWindow(GR_WINDOW_ID wid)
{
	GR_WINDOW	*wp;		/* window structure */

	wp = GsFindWindow(wid);
	if ((wp == NULL) || !wp->mapped)
		return;

	GsWpUnmapWindow(wp);

	wp->mapped = GR_FALSE;
}

/*
 * Clear the specified window.
 * This sets the window to its background color.
 * If the exposeflag is nonzero, then this also creates an exposure
 * event for the window.
 */
void GsClearWindow(GR_WINDOW_ID wid, GR_BOOL exposeflag)
{
	GR_WINDOW		*wp;	/* window structure */

	wp = GsPrepareWindow(wid);
	if (wp)
		GsWpClearWindow(wp, 0, 0, wp->width, wp->height, exposeflag);
}

/*
 * Set the focus to a particular window.
 * This makes keyboard events only visible to that window or children of it,
 * depending on the pointer location.
 */
void GsSetFocus(GR_WINDOW_ID wid)
{
	GR_WINDOW	*wp;		/* window structure */

	wp = GsFindWindow(wid);
	if (wp == NULL)
		return;

	if (wp->unmapcount) {
		GsError(GR_ERROR_UNMAPPED_FOCUS_WINDOW, wid);
		return;
	}

	focusfixed = (wp != rootwp);
	GsWpSetFocus(wp);
}

/*
 * Set the border of a window to the specified color.
 */
void GsSetBorderColor(GR_WINDOW_ID wid, GR_COLOR color)
{
	GR_WINDOW	*wp;		/* window structure */

	wp = GsFindWindow(wid);
	if ((wp == NULL) || (wp->bordercolor == color) ||
		(wp->bordersize == 0))
			return;

	wp->bordercolor = color;
	if (wp->unmapcount == 0)
		GsDrawBorder(wp);
}

/*
 * Specify a cursor for a window.
 * This cursor will only be used within that window, and by default
 * for its new children.  If the cursor is currently within this
 * window, it will be changed to the new one immediately.
 */
void GsSetCursor(GR_WINDOW_ID wid, GR_SIZE width, GR_SIZE height, GR_COORD hotx,
		GR_COORD hoty, GR_COLOR foreground, GR_COLOR background,
		GR_BITMAP *fgbitmap, GR_BITMAP *bgbitmap)
{
	GR_WINDOW	*wp;		/* window structure */
	GR_CURSOR	*cp;		/* cursor structure */
	int		bytes;		/* number of bytes of bitmap */

	wp = GsFindWindow(wid);
	if (wp == NULL)
		return;

	/*
	 * Make sure the size of the bitmap is reasonable.
	 */
	if ((width <= 0) || (width > MAX_CURSOR_SIZE) ||
		(height <= 0) || (height > MAX_CURSOR_SIZE))
	{
		GsError(GR_ERROR_BAD_CURSOR_SIZE, 0);
		return;
	}
	bytes = GR_BITMAP_SIZE(width, height) * sizeof(GR_BITMAP);

	/*
	 * See if the window is using a shared cursor definition.
	 * If so, then allocate a new private one, otherwise reuse it.
	 */
	cp = wp->cursor;
	if (cp == NULL || cp->usecount-- > 1) {
		cp = (GR_CURSOR *) malloc(sizeof(GR_CURSOR));
		if (cp == NULL) {
			GsError(GR_ERROR_MALLOC_FAILED, 0);
			return;
		}
	}

	cp->usecount = 1;
	cp->cursor.width = width;
	cp->cursor.height = height;
	cp->cursor.hotx = hotx;
	cp->cursor.hoty = hoty;
	cp->cursor.fgcolor = foreground;
	cp->cursor.bgcolor = background;
	memcpy(cp->cursor.image, fgbitmap, bytes);
	memcpy(cp->cursor.mask, bgbitmap, bytes);
	wp->cursor = cp;

	/*
	 * If this was the current cursor, then draw the new one.
	 */
	if (cp == curcursor || curcursor == NULL) {
		GdMoveCursor(cursorx - cp->cursor.hotx,
			cursory - cp->cursor.hoty);
		GdSetCursor(&cp->cursor);
	}
}

/*
 * Move the cursor to the specified absolute screen coordinates.
 * The coordinates are that of the defined hot spot of the cursor.
 * The cursor's appearance is changed to that defined for the window
 * in which the cursor is moved to.  In addition, mouse enter, mouse
 * exit, focus in, and focus out events are generated if necessary.
 */
void GsMoveCursor(GR_COORD x, GR_COORD y)
{
	/*
	 * Move the cursor only if necessary, offsetting it to
	 * place the hot spot at the specified coordinates.
	 */
	if ((x != cursorx) || (y != cursory)) {
		if(curcursor)
			GdMoveCursor(x - curcursor->cursor.hotx,
				y - curcursor->cursor.hoty);
		cursorx = x;
		cursory = y;
	}

	/*
	 * Now check to see which window the mouse is in, whether or
	 * not the cursor shape should be changed, and whether or not
	 * the input focus window should be changed.
	 */
	GsCheckMouseWindow();
	GsCheckFocusWindow();
	GsCheckCursor();
}

/*
 * Set the foreground color in a graphics context.
 */
void GsSetGCForeground(GR_GC_ID gc, GR_COLOR foreground)
{
	GR_GC		*gcp;		/* graphics context */

	gcp = GsFindGC(gc);
	if ((gcp == NULL) || (gcp->foreground == foreground))
		return;
	gcp->foreground = foreground;
	gcp->changed = GR_TRUE;
}

/*
 * Set the background color in a graphics context.
 */
void GsSetGCBackground(GR_GC_ID gc, GR_COLOR background)
{
	GR_GC		*gcp;		/* graphics context */

	gcp = GsFindGC(gc);
	if ((gcp == NULL) || (gcp->background == background))
		return;
	gcp->background = background;
	gcp->changed = GR_TRUE;
}

/*
 * Set whether or not the background color is drawn in bitmaps and text.
 */
void GsSetGCUseBackground(GR_GC_ID gc, GR_BOOL flag)
{
	GR_GC		*gcp;		/* graphics context */

	flag = (flag != 0);
	gcp = GsFindGC(gc);
	if ((gcp == NULL) || (gcp->usebackground == flag))
		return;
	gcp->usebackground = flag;
	gcp->changed = GR_TRUE;
}

/*
 * Set the drawing mode in a graphics context.
 */
void GsSetGCMode(GR_GC_ID gc, GR_MODE mode)
{
	GR_GC		*gcp;		/* graphics context */

	gcp = GsFindGC(gc);
	if ((gcp == NULL) || (gcp->mode == mode))
		return;
	if (mode > GR_MAX_MODE) {
		GsError(GR_ERROR_BAD_DRAWING_MODE, gc);
		return;
	}
	gcp->mode = mode;
	gcp->changed = GR_TRUE;
}

/*
 * Set the text font in a graphics context.
 */
void GsSetGCFont(GR_GC_ID gc, GR_FONT font)
{
	GR_GC		*gcp;		/* graphics context */

	gcp = GsFindGC(gc);
	if ((gcp == NULL) || (gcp->font == font))
		return;
	gcp->font = font;
	gcp->changed = GR_TRUE;
}

/*
 * Draw a line in the specified drawable using the specified graphics context.
 */

void GsLine(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x1, GR_COORD y1, GR_COORD x2,
			GR_COORD y2)
{
	GR_WINDOW	*wp;
	GR_PIXMAP	*pmp;

	switch (GsPrepareDrawing(id, gc, &wp, &pmp)) {
		case GR_DRAW_TYPE_WINDOW:
			GdLine(psd, wp->x + x1, wp->y + y1,
				wp->x + x2, wp->y + y2, TRUE);
			break;
	}
}

/*
 * Draw the boundary of a rectangle in the specified drawable using the
 * specified graphics context.
 */
void GsRect(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE width,
			GR_SIZE height)
{
	GR_WINDOW	*wp;
	GR_PIXMAP	*pmp;

	switch (GsPrepareDrawing(id, gc, &wp, &pmp)) {
		case GR_DRAW_TYPE_WINDOW:
			GdRect(psd, wp->x + x, wp->y + y, width, height);
			break;
	}
}

/*
 * Fill a rectangle in the specified drawable using the specified
 * graphics context.
 */
void GsFillRect(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE width,
			GR_SIZE height)
{
	GR_WINDOW	*wp;
	GR_PIXMAP	*pmp;

	switch (GsPrepareDrawing(id, gc, &wp, &pmp)) {
		case GR_DRAW_TYPE_WINDOW:
			GdFillRect(psd, wp->x + x, wp->y + y, width, height);
			break;
	}
}

/*
 * Draw the boundary of an ellipse in the specified drawable with
 * the specified graphics context.
 */
void GsEllipse(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE rx,
			GR_SIZE ry)
{
	GR_WINDOW	*wp;
	GR_PIXMAP	*pmp;

	switch (GsPrepareDrawing(id, gc, &wp, &pmp)) {
		case GR_DRAW_TYPE_WINDOW:
			GdEllipse(psd, wp->x + x, wp->y + y, rx, ry);
			break;
	}
}

/*
 * Fill an ellipse in the specified drawable using the specified
 * graphics context.
 */
void GsFillEllipse(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE rx,
			GR_SIZE ry)
{
	GR_WINDOW	*wp;
	GR_PIXMAP	*pmp;

	switch (GsPrepareDrawing(id, gc, &wp, &pmp)) {
		case GR_DRAW_TYPE_WINDOW:
			GdFillEllipse(psd, wp->x + x, wp->y + y, rx, ry);
			break;
	}
}

/*
 * Draw a rectangular area in the specified drawable using the specified
 * graphics, as determined by the specified bit map.  This differs from
 * rectangle drawing in that the rectangle is drawn using the foreground
 * color and possibly the background color as determined by the bit map.
 * Each row of bits is aligned to the next bitmap word boundary (so there
 * is padding at the end of the row).  The background bit values are only
 * written if the usebackground flag is set in the GC.
 */
void GsBitmap(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE width,
	GR_SIZE height, GR_BITMAP *imagebits)
{
	GR_WINDOW	*wp;
	GR_PIXMAP	*pmp;

	switch (GsPrepareDrawing(id, gc, &wp, &pmp)) {
		case GR_DRAW_TYPE_WINDOW:
			GdBitmap(psd, wp->x + x, wp->y + y, width, height,
				imagebits);
			break;
	}
}

/*
 * Draw a rectangular area in the specified drawable using the specified
 * graphics context.  This differs from rectangle drawing in that the
 * color values for each pixel in the rectangle are specified.
 * The color table is indexed row by row.
 */
void GsArea(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_SIZE width,
	GR_SIZE height, PIXELVAL *pixels)
{
	GR_WINDOW	*wp;
	GR_PIXMAP	*pmp;

	switch (GsPrepareDrawing(id, gc, &wp, &pmp)) {
		case GR_DRAW_TYPE_WINDOW:
			GdArea(psd, wp->x + x, wp->y + y, width, height,pixels);
			break;
	}
}

/*
 * Read the color values from the specified rectangular area of the
 * specified drawable into a supplied buffer.  If the drawable is a
 * window which is obscured by other windows, then the returned values
 * will include the values from the covering windows.  Regions outside
 * of the screen boundaries, or unmapped windows will return black.
 */
void GsReadArea(GR_DRAW_ID id,GR_COORD x,GR_COORD y,GR_SIZE width,
	GR_SIZE height,PIXELVAL *pixels)
{
	GR_WINDOW	*wp;
	long		count;
	PIXELVAL	black;

	wp = GsFindWindow(id);			/* FIXME- should check pixmap */
	if ((wp == NULL) || wp->unmapcount || (x >= wp->width) ||
	    (y >= wp->height) || (x + width <= 0) || (y + height <= 0)) {
		black = GdFindColor(BLACK);
		count = width * height;
		while (count-- > 0)
			*pixels++ = black;
		return;
	}
	GdReadArea(psd, wp->x, wp->y, width, height, pixels);
}

/*
 * Draw a point in the specified drawable using the specified
 * graphics context.
 */
void GsPoint(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y)
{
	GR_WINDOW	*wp;
	GR_PIXMAP	*pmp;

	switch (GsPrepareDrawing(id, gc, &wp, &pmp)) {
		case GR_DRAW_TYPE_WINDOW:
			GdPoint(psd, wp->x + x, wp->y + y);
			break;
	}
}

/*
 * Draw a polygon in the specified drawable using the specified
 * graphics context.  The polygon is only complete if the first
 * point is repeated at the end.
 */
void GsPoly(GR_DRAW_ID id, GR_GC_ID gc, GR_COUNT count, GR_POINT *pointtable)
{
	GR_WINDOW	*wp;
	GR_PIXMAP	*pmp;
	GR_POINT	*pp;
	GR_COUNT	i;

	switch (GsPrepareDrawing(id, gc, &wp, &pmp)) {
		case GR_DRAW_TYPE_WINDOW:
			break;
		default:
			return;
	}

	/*
	 * Here for drawing to a window.
	 * Relocate all the points relative to the window.
	 */
	pp = pointtable;
	for (i = count; i-- > 0; pp++) {
		pp->x += wp->x;
		pp->y += wp->y;
	}

	GdPoly(psd, count, pointtable);

	/*
	 * The following is temporarily necessary until the server
	 * becomes a separate process.  We don't want to change the
	 * user's arguments!
	 */
	pp = pointtable;
	for (i = count; i-- > 0; pp++) {
		pp->x -= wp->x;
		pp->y -= wp->y;
	}
}

/*
 * Draw a filled polygon in the specified drawable using the specified
 * graphics context.  The last point may be a duplicate of the first
 * point, but this is not required.
 */
void GsFillPoly(GR_DRAW_ID id, GR_GC_ID gc, GR_COUNT count, GR_POINT *pointtable)
{
	GR_WINDOW	*wp;
	GR_PIXMAP	*pmp;
	GR_POINT	*pp;
	GR_COUNT	i;

	switch (GsPrepareDrawing(id, gc, &wp, &pmp)) {
		case GR_DRAW_TYPE_WINDOW:
			break;
		default:
			return;
	}

	/*
	 * Here for drawing to a window.
	 * Relocate all the points relative to the window.
	 */
	pp = pointtable;
	for (i = count; i-- > 0; pp++) {
		pp->x += wp->x;
		pp->y += wp->y;
	}

	GdFillPoly(psd, count, pointtable);

	/*
	 * The following is temporarily necessary until the server
	 * becomes a separate process.  We don't want to change the
	 * user's arguments!
	 */
	pp = pointtable;
	for (i = count; i-- > 0; pp++) {
		pp->x -= wp->x;
		pp->y -= wp->y;
	}
}

/*
 * Draw a text string in the specified drawable using the
 * specified graphics context.
 */
void GsText(GR_DRAW_ID id, GR_GC_ID gc, GR_COORD x, GR_COORD y, GR_CHAR *str,
			GR_COUNT count)
{
	GR_WINDOW	*wp;
	GR_PIXMAP	*pmp;

	switch (GsPrepareDrawing(id, gc, &wp, &pmp)) {
		case GR_DRAW_TYPE_WINDOW:
			GdText(psd, wp->x + x, wp->y + y, str, count, TRUE);
			break;
	}
}
