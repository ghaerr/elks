/*
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Graphics server utility routines for windows.
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "serv.h"

extern PSD psd;	/* temporary*/

/*
 * Help prevent future bugs by defining this variable to an illegal value.
 * These routines should not be referencing this, but should be using
 * unmapcount instead.
 */
#define	mapped	cannotusemapped

/*
 * Macro to distinguish cases of clipping.
 */
#define	GAPVAL(leftgap, rightgap, topgap, bottomgap) \
	(((leftgap) << 3) + ((rightgap) << 2) + ((topgap) << 1) + (bottomgap))


/*
 * Unmap the window to make it and its children invisible on the screen.
 * This is a recursive routine which increments the unmapcount values for
 * this window and all of its children, and causes exposure events for
 * windows which are newly uncovered.
 */
void GsWpUnmapWindow(GR_WINDOW *wp)
{
	GR_WINDOW	*pwp;		/* parent window */
	GR_WINDOW	*sibwp;		/* sibling window */
	GR_WINDOW	*childwp;	/* child window */
	GR_SIZE		bs;		/* border size of this window */

	if (wp == rootwp) {
		GsError(GR_ERROR_ILLEGAL_ON_ROOT_WINDOW, wp->id);
		return;
	}

	if (wp == clipwp)
		clipwp = NULL;

	wp->unmapcount++;

	for (childwp = wp->children; childwp; childwp = childwp->siblings)
		GsWpUnmapWindow(childwp);

	if (wp == mousewp) {
		GsCheckMouseWindow();
		GsCheckCursor();
	}

	if (wp == focuswp) {
		focusfixed = GR_FALSE;
		GsCheckFocusWindow();
	}

	/*
	 * If this is an input-only window or the parent window is
	 * still unmapped, then we are all done.
	 */
	if (!wp->output || wp->parent->unmapcount)
		return;

	/*
	 * Clear the area in the parent for this window, causing an
	 * exposure event for it.  Take into account the border size.
	 */
	bs = wp->bordersize;
	pwp = wp->parent;
	GsWpClearWindow(pwp, wp->x - pwp->x - bs, wp->y - pwp->y - bs,
		wp->width + bs * 2, wp->height + bs * 2, GR_TRUE);

	/*
	 * Finally clear and redraw all parts of our lower sibling
	 * windows that were covered by this window.
	 */
	sibwp = wp;
	while (sibwp->siblings) {
		sibwp = sibwp->siblings;
		GsExposeArea(sibwp, wp->x - bs, wp->y - bs,
			wp->width + bs * 2, wp->height + bs * 2);
	}
}

/*
 * Map the window to possibly make it and its children visible on the screen.
 * This is a recursive routine which decrements the unmapcount values for
 * this window and all of its children, and causes exposure events for
 * those windows which become visible.
 */
void GsWpMapWindow(GR_WINDOW *wp)
{
	if (wp == rootwp) {
		GsError(GR_ERROR_ILLEGAL_ON_ROOT_WINDOW, wp->id);
		return;
	}

	if (wp->unmapcount)
		wp->unmapcount--;

	if (wp->unmapcount == 0) {
		GsCheckMouseWindow();
		GsCheckFocusWindow();
		GsCheckCursor();
	}

	/*
	 * If the window is an output window and just became visible,
	 * then draw its border, clear it to the background color, and
	 * generate an exposure event.
	 */
	if (wp->output && (wp->unmapcount == 0)) {
		GsDrawBorder(wp);
		GsWpClearWindow(wp, 0, 0, wp->width, wp->height, GR_TRUE);
	}

	/*
	 * Do the same thing for the children.
	 */
	for (wp = wp->children; wp; wp = wp->siblings)
		GsWpMapWindow(wp);
}

/*
 * Destroy the specified window, and all of its children.
 * This is a recursive routine.
 */
void GsWpDestroyWindow(GR_WINDOW *wp)
{
	GR_WINDOW	*prevwp;	/* previous window pointer */
	GR_EVENT_CLIENT	*ecp;		/* selections for window */

	if (wp == rootwp) {
		GsError(GR_ERROR_ILLEGAL_ON_ROOT_WINDOW, wp->id);
		return;
	}

	/*
	 * Unmap the window first.
	 */
	if (wp->unmapcount == 0)
		GsWpUnmapWindow(wp);

	/*
	 * Destroy all children.
	 */
	while (wp->children)
		GsWpDestroyWindow(wp->children);

	/*
	 * Free all client selection structures.
	 */
	while (wp->eventclients) {
		ecp = wp->eventclients;
		wp->eventclients = ecp->next;
		free(ecp);
	}

	/*
	 * Free any cursor associated with the window.
	 */
	if (wp->cursor->usecount-- == 1) {
		free(wp->cursor);
		wp->cursor = NULL;
	}

	/*
	 * Remove this window from the child list of its parent.
	 */
	prevwp = wp->parent->children;
	if (prevwp == wp)
		wp->parent->children = wp->siblings;
	else {
		while (prevwp->siblings != wp)
			prevwp = prevwp->siblings;
		prevwp->siblings = wp->siblings;
	}
	wp->siblings = NULL;

	/*
	 * Remove this window from the complete list of windows.
	 */
	prevwp = listwp;
	if (prevwp == wp)
		listwp = wp->next;
	else {
		while (prevwp->next != wp)
			prevwp = prevwp->next;
		prevwp->next = wp->next;
	}
	wp->next = NULL;

	/*
	 * Forget various information if they related to this window.
	 * Then finally free the structure.
	 */
	if (wp == clipwp)
		clipwp = NULL;
	if (wp == grabbuttonwp)
		grabbuttonwp = NULL;
	if (wp == cachewp) {
		cachewindowid = 0;
		cachewp = NULL;
	}
	if (wp == focuswp) {
		focusfixed = GR_FALSE;
		focuswp = rootwp;
	}

	free(wp);
}

/*
 * Clear the specified area of a window and possibly make an exposure event.
 * This sets the area window to its background color.  If the exposeflag is
 * nonzero, then this also creates an exposure event for the window.
 */
void GsWpClearWindow(GR_WINDOW *wp, GR_COORD x, GR_COORD y, GR_SIZE width,
			GR_SIZE  height, GR_BOOL exposeflag)
{
	if (wp->unmapcount || !wp->output)
		return;

	/*
	 * Reduce the arguments so that they actually lie within the window.
	 */
	if (x < 0) {
		width += x;
		x = 0;
	}
	if (y < 0) {
		height += y;
		y = 0;
	}
	if (x + width > wp->width)
		width = wp->width - x;
	if (y + height > wp->height)
		height = wp->height - y;

	/*
	 * Now see if the region is really in the window.  If not, then
	 * do nothing.
	 */
	if ((x >= wp->width) || (y >= wp->height) || (width <= 0) ||
		(height <= 0))
			return;

	/*
	 * Draw the background of the window.
	 * Invalidate the current graphics context since
	 * we are changing the foreground color and mode.
	 */
	GsSetClipWindow(wp);
	curgcp = NULL;
	GdSetMode(GR_MODE_SET);
	GdSetForeground(GdFindColor(wp->background));
	GdFillRect(psd, wp->x + x, wp->y + y, width, height);

	/*
	 * Now do the exposure if required.
	 */
	if (exposeflag)
		GsDeliverExposureEvent(wp, x, y, width, height);
}

/*
 * Handle the exposing of the specified absolute region of the screen,
 * starting with the specified window.  That window and all of its
 * children will be redrawn and/or exposure events generated if they
 * overlap the specified area.  This is a recursive routine.
 */
void GsExposeArea(GR_WINDOW *wp, GR_COORD rootx, GR_COORD rooty, GR_SIZE width, GR_SIZE height)
{
	if (!wp->output || wp->unmapcount)
		return;

	/*
	 * First see if the area overlaps the window including the border.
	 * If not, then there is nothing more to do.
	 */
	if ((rootx >= wp->x + wp->width + wp->bordersize) ||
		(rooty >= wp->y + wp->height + wp->bordersize) ||
		(rootx + width <= wp->x - wp->bordersize) ||
		(rooty + height <= wp->y - wp->bordersize))
			return;

	/*
	 * The area does overlap the window.  See if the area overlaps
	 * the border, and if so, then redraw it.
	 */
	if ((rootx < wp->x) || (rooty < wp->y) ||
		(rootx + width > wp->x + wp->width) ||
		(rooty + height > wp->y + wp->height))
			GsDrawBorder(wp);

	/*
	 * Now clear the window itself in the specified area,
	 * which might cause an exposure event.
	 */
	GsWpClearWindow(wp, rootx - wp->x, rooty - wp->y,
		width, height, GR_TRUE);

	/*
	 * Now do the same for all the children.
	 */
	for (wp = wp->children; wp; wp = wp->siblings)
		GsExposeArea(wp, rootx, rooty, width, height);
}

/*
 * Draw the border of a window if there is one.
 * Note: To allow the border to be drawn with the correct clipping,
 * we temporarily grow the size of the window to include the border.
 */
void GsDrawBorder(GR_WINDOW *wp)
{
	GR_COORD	lminx;		/* left edge minimum x */
	GR_COORD	rminx;		/* right edge minimum x */
	GR_COORD	tminy;		/* top edge minimum y */
	GR_COORD	bminy;		/* bottom edge minimum y */
	GR_COORD	topy;		/* top y value of window */
	GR_COORD	boty;		/* bottom y value of window */
	GR_SIZE		width;		/* original width of window */
	GR_SIZE		height;		/* original height of window */
	GR_SIZE		bs;		/* border size */

	bs = wp->bordersize;
	if (bs <= 0)
		return;

	width = wp->width;
	height = wp->height;
	lminx = wp->x - bs;
	rminx = wp->x + width;
	tminy = wp->y - bs;
	bminy = wp->y + height;
	topy = wp->y;
	boty = bminy - 1;
 
	wp->x -= bs;
	wp->y -= bs;
	wp->width += (bs * 2);
	wp->height += (bs * 2);
	wp->bordersize = 0;

	clipwp = NULL;
	GsSetClipWindow(wp);
	curgcp = NULL;
	GdSetMode(GR_MODE_SET);
	GdSetForeground(GdFindColor(wp->bordercolor));

	if (bs == 1) {
		GdLine(psd, lminx, tminy, rminx, tminy, TRUE);
		GdLine(psd, lminx, bminy, rminx, bminy, TRUE);
		GdLine(psd, lminx, topy, lminx, boty, TRUE);
		GdLine(psd, rminx, topy, rminx, boty, TRUE);
	} else {
		GdFillRect(psd, lminx, tminy, width + bs * 2, bs);
		GdFillRect(psd, lminx, bminy, width + bs * 2, bs);
		GdFillRect(psd, lminx, topy, bs, height);
		GdFillRect(psd, rminx, topy, bs, height);
	}

	/*
	 * Restore the true window size.
	 * Forget the currently clipped window since we messed it up.
	 */
	wp->x += bs;
	wp->y += bs;
	wp->width -= (bs * 2);
	wp->height -= (bs * 2);
	wp->bordersize = bs;
	clipwp = NULL;
}

/*
 * Check to see if the first window overlaps the second window.
 */
GR_BOOL GsCheckOverlap(GR_WINDOW *topwp, GR_WINDOW *botwp)
{
	GR_COORD	minx1;
	GR_COORD	miny1;
	GR_COORD	maxx1;
	GR_COORD	maxy1;
	GR_COORD	minx2;
	GR_COORD	miny2;
	GR_COORD	maxx2;
	GR_COORD	maxy2;
	GR_SIZE		bs;

	if (!topwp->output || topwp->unmapcount || botwp->unmapcount)
		return GR_FALSE;

	bs = topwp->bordersize;
	minx1 = topwp->x - bs;
	miny1 = topwp->y - bs;
	maxx1 = topwp->x + topwp->width + bs - 1;
	maxy1 = topwp->y + topwp->height + bs - 1;

	bs = botwp->bordersize;
	minx2 = botwp->x - bs;
	miny2 = botwp->y - bs;
	maxx2 = botwp->x + botwp->width + bs - 1;
	maxy2 = botwp->y + botwp->height + bs - 1;

	if ((minx1 > maxx2) || (minx2 > maxx1) || (miny1 > maxy2)
		|| (miny2 > maxy1))
			return GR_FALSE;

	return GR_TRUE;
}

/*
 * Return a pointer to the window structure with the specified window id.
 * Returns NULL if the window does not exist, with an error set.
 */
GR_WINDOW *GsFindWindow(GR_WINDOW_ID id)
{
	GR_WINDOW	*wp;		/* current window pointer */

	/*
	 * See if this is the root window or the same window as last time.
	 */
	if (id == GR_ROOT_WINDOW_ID)
		return rootwp;

	if ((id == cachewindowid) && id)
		return cachewp;

	/*
	 * No, search for it and cache it for future calls.
	 */
	for (wp = listwp; wp; wp = wp->next) {
		if (wp->id == id) {
			cachewindowid = id;
			cachewp = wp;
			return wp;
		}
	}

	GsError(GR_ERROR_BAD_WINDOW_ID, id);

	return NULL;
}

/*
 * Return a pointer to the graphics context with the specified id.
 * Returns NULL if the graphics context does not exist, with an
 * error saved.
 */
GR_GC *GsFindGC(GR_GC_ID gcid)
{
	GR_GC		*gcp;		/* current graphics context pointer */

	/*
	 * See if this is the same graphics context as last time.
	 */
	if ((gcid == cachegcid) && gcid)
		return cachegcp;

	/*
	 * No, search for it and cache it for future calls.
	 */
	for (gcp = listgcp; gcp; gcp = gcp->next) {
		if (gcp->id == gcid) {
			cachegcid = gcid;
			cachegcp = gcp;
			return gcp;
		}
	}

	GsError(GR_ERROR_BAD_GC_ID, gcid);

	return NULL;
}

/*
 * Prepare to do drawing in a window or pixmap using the specified
 * graphics context.  Returns the window or pixmap pointer if successful,
 * and the type of drawing id that was supplied.  Returns the special value
 * GR_DRAW_TYPE_NONE if an error is generated, or if drawing is useless.
 */
GR_DRAW_TYPE GsPrepareDrawing(GR_DRAW_ID id, GR_GC_ID gcid, GR_WINDOW **retwp,
				GR_PIXMAP **retpixptr)
{
	GR_WINDOW	*wp;		/* found window */
	GR_GC		*gcp;		/* found graphics context */

	*retwp = NULL;
	*retpixptr = NULL;

	gcp = GsFindGC(gcid);
	if (gcp == NULL)
		return GR_DRAW_TYPE_NONE;

	/*
	 * Future: look for pixmap id too.
	 * For now, can only have window ids.
	 */
	wp = GsFindWindow(id);
	if (wp == NULL)
		return GR_DRAW_TYPE_NONE;

	if (!wp->output) {
		GsError(GR_ERROR_INPUT_ONLY_WINDOW, id);
		return GR_DRAW_TYPE_NONE;
	}

	if (wp->unmapcount)
		return GR_DRAW_TYPE_NONE;

	/*
	 * If the window is not the currently clipped one, then make it the
	 * current one and define its clip rectangles.
	 */
	if (wp != clipwp)
		GsSetClipWindow(wp);

	/*
	 * If the graphics context is not the current one, then
	 * make it the current one and remember to update it.
	 */
	if (gcp != curgcp) {
		curgcp = gcp;
		gcp->changed = GR_TRUE;
	}

	/*
	 * If the graphics context has been changed, then tell the
	 * device driver about it.
	 */
	if (gcp->changed) {
		GdSetForeground(GdFindColor(gcp->foreground));
		GdSetBackground(GdFindColor(gcp->background));
		GdSetMode(gcp->mode);
		GdSetUseBackground(gcp->usebackground);
		GdSetFont(gcp->font);
		gcp->changed = GR_FALSE;
	}

	*retwp = wp;
	return GR_DRAW_TYPE_WINDOW;
}

/*
 * Prepare the specified window for drawing into it.
 * This sets up the clipping regions to just allow drawing into it.
 * Returns NULL if the drawing is illegal (with an error generated),
 * or if the window is not mapped.
 */
GR_WINDOW *GsPrepareWindow(GR_WINDOW_ID wid)
{
	GR_WINDOW	*wp;		/* found window */

	wp = GsFindWindow(wid);
	if (wp == NULL)
		return NULL;
	
	if (!wp->output) {
		GsError(GR_ERROR_INPUT_ONLY_WINDOW, wid);
		return NULL;
	}

	if (wp->unmapcount)
		return NULL;

	if (wp != clipwp)
		GsSetClipWindow(wp);

	return wp;
}

/*
 * Set the clip rectangles for a window taking into account other
 * windows that may be obscuring it.  The windows that may be obscuring
 * this one are the siblings of each direct ancestor which are higher
 * in priority than those ancestors.  Also, each parent limits the visible
 * area of the window.  The clipping is not done if it is already up to
 * date of if the window is not outputtable.
 */
void GsSetClipWindow(GR_WINDOW *wp)
{
	GR_WINDOW	*pwp;		/* parent window */
	GR_WINDOW	*sibwp;		/* sibling windows */
	CLIPRECT	*clip;		/* first clip rectangle */
	GR_COUNT	count;		/* number of clip rectangles */
	GR_COUNT	newcount;	/* number of new rectangles */
	GR_COUNT	i;		/* current index */
	GR_COORD	minx;		/* minimum clip x coordinate */
	GR_COORD	miny;		/* minimum clip y coordinate */
	GR_COORD	maxx;		/* maximum clip x coordinate */
	GR_COORD	maxy;		/* maximum clip y coordinate */
	GR_COORD	diff;		/* difference in coordinates */
	GR_SIZE		bs;		/* border size */
	GR_BOOL		toomany;	/* TRUE if too many clip rects */
	CLIPRECT	cliprects[MAX_CLIPRECTS];	/* clip rectangles */

	if (wp->unmapcount || !wp->output || (wp == clipwp))
		return;

	clipwp = wp;

	/*
	 * Start with the rectangle for the complete window.
	 * We will then cut pieces out of it as needed.
	 */
	count = 1;
	clip = cliprects;
	clip->x = wp->x;
	clip->y = wp->y;
	clip->width = wp->width;
	clip->height = wp->height;

	/*
	 * First walk upwards through all parent windows,
	 * and restrict the visible part of this window to the part
	 * that shows through all of those parent windows.
	 */
	pwp = wp;
	while (pwp != rootwp) {
		pwp = pwp->parent;

		diff = pwp->x - clip->x;
		if (diff > 0) {
			clip->width -= diff;
			clip->x = pwp->x;
		}

		diff = (pwp->x + pwp->width) - (clip->x + clip->width);
		if (diff < 0)
			clip->width += diff;

		diff = pwp->y - clip->y;
		if (diff > 0) {
			clip->height -= diff;
			clip->y = pwp->y;
		}

		diff = (pwp->y + pwp->height) - (clip->y + clip->height);
		if (diff < 0)
			clip->height += diff;
	}

	/*
	 * If the window is completely clipped out of view, then
	 * set the clipping region to indicate that.
	 */
	if ((clip->width <= 0) || (clip->height <= 0)) {
		GdSetClipRects(1, cliprects);
		return;
	}

	/*
	 * Now examine all windows that obscure this window, and
	 * for each obscuration, break up the clip rectangles into
	 * the smaller pieces that are still visible.  The windows
	 * that can obscure us are the earlier siblings of all of
	 * our parents.
 	 */
	toomany = GR_FALSE;
	pwp = wp;
	while (pwp != rootwp) {
		wp = pwp;
		pwp = wp->parent;
		sibwp = pwp->children;
		for (; sibwp != wp; sibwp = sibwp->siblings) {
			if (sibwp->unmapcount || !sibwp->output)
				continue;

			bs = sibwp->bordersize;
			minx = sibwp->x - bs;
			miny = sibwp->y - bs;
			maxx = sibwp->x + sibwp->width + bs - 1;
			maxy = sibwp->y + sibwp->height + bs - 1;

			newcount = count;
			for (i = 0; i < count; i++) {
				if (newcount > MAX_CLIPRECTS - 3) {
					toomany = GR_TRUE;
					break;
				}
				newcount += GsSplitClipRect(&cliprects[i],
					&cliprects[newcount],
					minx, miny, maxx, maxy);
			}
			count = newcount;
		}
	}

	if (toomany) {
		GsError(GR_ERROR_TOO_MUCH_CLIPPING, wp->id);
		clip->x = 0;
		clip->y = 0;
		clip->width = -1;
		clip->height = -1;
		count = 1;
	}

	/*
	 * Set the clip rectangles.
	 */
	GdSetClipRects(count, (CLIPRECT *)cliprects);
}

/*
 * Check the specified clip rectangle against the specified rectangular
 * region, and reduce it or split it up into multiple clip rectangles
 * such that the specified region is not contained in any of the clip
 * rectangles.  The source clip rectangle can be modified in place, and
 * in addition more clip rectangles can be generated, which are placed in
 * the indicated destination location.  The maximum number of new clip
 * rectangles needed is 3.  Returns the number of clip rectangles added.
 * If the source clip rectangle is totally obliterated, it is set to an
 * impossible region and 0 is returned.  When splits are done, we prefer
 * to create wide regions instead of high regions.
 */
GR_COUNT GsSplitClipRect(CLIPRECT *srcrect, CLIPRECT *destrect, GR_COORD minx, GR_COORD miny,
			GR_COORD maxx, GR_COORD maxy)
{
	GR_COORD	x;
	GR_COORD	y;
	GR_SIZE		width;
	GR_SIZE		height;
	GR_COORD	dx;
	GR_COORD	dy;
	int		gaps;

	/*
	 * First see if there is any overlap at all.
	 * If not, then nothing to do.
	 */
	x = srcrect->x;
	y = srcrect->y;
	width = srcrect->width;
	height = srcrect->height;

	if ((minx > maxx) || (miny > maxy) || (maxx < x) || (maxy < y) ||
		(x + width <= minx) || (y + height <= miny))
			return 0;

	/*
	 * There is an overlap.  Calculate a value to differentiate
	 * various cases, and then handle each case separately.  The
	 * cases are classified on whether there are gaps on the left,
	 * right, top, and bottom sides of the clip rectangle.
	 */
	gaps = 0;
	if (x < minx)
		gaps |= GAPVAL(1, 0, 0, 0);
	if (x + width - 1 > maxx)
		gaps |= GAPVAL(0, 1, 0, 0);
	if (y < miny)
		gaps |= GAPVAL(0, 0, 1, 0);
	if (y + height - 1 > maxy)
		gaps |= GAPVAL(0, 0, 0, 1);

	switch (gaps) {
		case GAPVAL(0, 0, 0, 0):	/* no gaps at all */
			srcrect->x = 0;
			srcrect->y = 0;
			srcrect->width = 0;
			srcrect->height = 0;
			return 0;

		case GAPVAL(0, 0, 0, 1):	/* gap on bottom */
			dy = maxy - y + 1;
			srcrect->y += dy;
			srcrect->height -= dy;
			return 0;

		case GAPVAL(0, 0, 1, 0):	/* gap on top */
			srcrect->height = miny - y;
			return 0;

		case GAPVAL(0, 0, 1, 1):	/* gap on top, bottom */
			srcrect->height = miny - y;
			destrect->x = x;
			destrect->width = width;
			destrect->y = maxy + 1;
			destrect->height = y + height - maxy - 1;
			return 1;

		case GAPVAL(0, 1, 0, 0):	/* gap on right */
			dx = maxx - x + 1;
			srcrect->x += dx;
			srcrect->width -= dx;
			return 0;

		case GAPVAL(0, 1, 0, 1):	/* gap on right, bottom */
			dx = maxx - x + 1;
			srcrect->x += dx;
			srcrect->width -= dx;
			srcrect->height = maxy - y + 1;
			destrect->x = x;
			destrect->width = width;
			destrect->y = maxy + 1;
			destrect->height = y + height - maxy - 1;
			return 1;

		case GAPVAL(0, 1, 1, 0):	/* gap on right, top */
			dx = maxx - x + 1;
			srcrect->height = miny - y;
			destrect->x = x + dx;
			destrect->width = width - dx;
			destrect->y = miny;
			destrect->height = y + height - miny;
			return 1;

		case GAPVAL(0, 1, 1, 1):	/* gap on right, top, bottom */
			dx = maxx - x + 1;
			srcrect->height = miny - y;
			destrect->x = x;
			destrect->width = width;
			destrect->y = maxy + 1;
			destrect->height = y + height - maxy - 1;
			destrect++;
			destrect->x = x + dx;
			destrect->width = width - dx;
			destrect->y = miny;
			destrect->height = maxy - miny + 1;
			return 2;

		case GAPVAL(1, 0, 0, 0):	/* gap on left */
			srcrect->width = minx - x;
			return 0;

		case GAPVAL(1, 0, 0, 1):	/* gap on left, bottom */
			srcrect->width = minx - x;
			srcrect->height = maxy - y + 1;
			destrect->x = x;
			destrect->width = width;
			destrect->y = maxy + 1;
			destrect->height = y + height - maxy - 1;
			return 1;

		case GAPVAL(1, 0, 1, 0):	/* gap on left, top */
			srcrect->height = miny - y;
			destrect->x = x;
			destrect->width = minx - x;
			destrect->y = miny;
			destrect->height = y + height - miny;
			return 1;

		case GAPVAL(1, 0, 1, 1):	/* gap on left, top, bottom */
			srcrect->height = miny - y;
			destrect->x = x;
			destrect->width = minx - x;
			destrect->y = miny;
			destrect->height = maxy - miny + 1;
			destrect++;
			destrect->x = x;
			destrect->width = width;
			destrect->y = maxy + 1;
			destrect->height = y + height - maxy - 1;
			return 2;

		case GAPVAL(1, 1, 0, 0):	/* gap on left, right */
			destrect->x = maxx + 1;
			destrect->width = x + width - maxx - 1;
			destrect->y = y;
			destrect->height = height;
			srcrect->width = minx - x;
			return 1;

		case GAPVAL(1, 1, 0, 1):	/* gap on left, right, bottom */
			dy = maxy - y + 1;
			srcrect->y += dy;
			srcrect->height -= dy;
			destrect->x = x;
			destrect->width = minx - x;
			destrect->y = y;
			destrect->height = dy;
			destrect++;
			destrect->x = maxx + 1;
			destrect->width = x + width - maxx - 1;
			destrect->y = y;
			destrect->height = dy;
			return 2;

		case GAPVAL(1, 1, 1, 0):	/* gap on left, right, top */
			srcrect->height = miny - y;
			destrect->x = x;
			destrect->width = minx - x;
			destrect->y = miny;
			destrect->height = y + height - miny;
			destrect++;
			destrect->x = maxx + 1;
			destrect->width = x + width - maxx - 1;
			destrect->y = miny;
			destrect->height = y + height - miny;
			return 2;

		case GAPVAL(1, 1, 1, 1):	/* gap on all sides */
			srcrect->height = miny - y;
			destrect->x = x;
			destrect->width = minx - x;
			destrect->y = miny;
			destrect->height = maxy - miny + 1;
			destrect++;
			destrect->x = maxx + 1;
			destrect->width = x + width - maxx - 1;
			destrect->y = miny;
			destrect->height = maxy - miny + 1;
			destrect++;
			destrect->x = x;
			destrect->width = width;
			destrect->y = maxy + 1;
			destrect->height = y + height - maxy - 1;
			return 3;
	}
	return 0; /* NOTREACHED */
}

/*
 * Find the window which is currently visible for the specified coordinates.
 * This just walks down the window tree looking for the deepest mapped
 * window which contains the specified point.  If the coordinates are
 * off the screen, the root window is returned.
 */
GR_WINDOW *GsFindVisibleWindow(GR_COORD x, GR_COORD y)
{
	GR_WINDOW	*wp;		/* current window */
	GR_WINDOW	*retwp;		/* returned window */

	wp = rootwp;
	retwp = wp;
	while (wp) {
		if ((wp->unmapcount == 0) && (wp->x <= x) && (wp->y <= y) &&
			(wp->x + wp->width > x) && (wp->y + wp->height > y))
		{
			retwp = wp;
			wp = wp->children;
			continue;
		}
		wp = wp->siblings;
	}
	return retwp;
}

/*
 * Check to see if the cursor shape is the correct shape for its current
 * location.  If not, its shape is changed.
 */
void GsCheckCursor(void)
{
	GR_WINDOW	*wp;		/* window cursor is in */
	GR_CURSOR	*cp;		/* cursor definition */

	/*
	 * Get the cursor at its current position, and if it is not the
	 * currently defined one, then set the new cursor.  However,
	 * if the pointer is currently grabbed, then leave it alone.
	 */
	wp = grabbuttonwp;
	if (wp == NULL)
		wp = mousewp;

	cp = wp->cursor;
	if (cp == curcursor)
		return;

	/*
	 * It needs redefining, so do it.
	 */
	curcursor = cp;
	GdMoveCursor(cursorx - cp->cursor.hotx, cursory - cp->cursor.hoty);
	GdSetCursor(&cp->cursor);
}

/*
 * Check to see if the window the mouse is currently in has changed.
 * If so, generate enter and leave events as required.  The newest
 * mouse window is remembered in mousewp.  However, do not change the
 * window while it is grabbed.
 */
void GsCheckMouseWindow(void)
{
	GR_WINDOW	*wp;		/* newest window for mouse */

	wp = grabbuttonwp;
	if (wp == NULL)
		wp = GsFindVisibleWindow(cursorx, cursory);
	if (wp == mousewp)
		return;

	GsDeliverGeneralEvent(mousewp, GR_EVENT_TYPE_MOUSE_EXIT);

	mousewp = wp;

	GsDeliverGeneralEvent(wp, GR_EVENT_TYPE_MOUSE_ENTER);
}

/*
 * Determine the current focus window for the current mouse coordinates.
 * The mouse coordinates only matter if the focus is not fixed.  Otherwise,
 * the selected window is dependant on the window which wants keyboard
 * events.  This also sets the current focus for that found window.
 * The window with focus is remembered in focuswp.
 */
void GsCheckFocusWindow(void)
{
	GR_WINDOW		*wp;		/* current window */
	GR_EVENT_CLIENT		*ecp;		/* current event client */
	GR_EVENT_MASK		eventmask;	/* event mask */

	if (focusfixed)
		return;

	eventmask = GR_EVENT_MASK_KEY_DOWN;

	/*
	 * Walk upwards from the current window containing the mouse
	 * looking for the first window which would accept a keyboard event.
	 */
	for (wp = mousewp; ;wp = wp->parent) {
		for (ecp = wp->eventclients; ecp; ecp = ecp->next) {
			if (ecp->eventmask & eventmask) {
				GsWpSetFocus(wp);
				return;
			}
		}
		if ((wp == rootwp) || (wp->nopropmask & eventmask)) {
			GsWpSetFocus(rootwp);
			return;
		}
	}
}

/*
 * Set the input focus to the specified window.
 * This generates focus out and focus in events as necessary.
 */
void GsWpSetFocus(GR_WINDOW *wp)
{
	if (wp == focuswp)
		return;

	GsDeliverGeneralEvent(focuswp, GR_EVENT_TYPE_FOCUS_OUT);

	focuswp = wp;

	GsDeliverGeneralEvent(wp, GR_EVENT_TYPE_FOCUS_IN);
}
