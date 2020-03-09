/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Device-independent routines to determine clipping regions.
 */
#include <stdio.h>
#include "device.h"

/* Clip cache rectangle information.
 * After calling GdClipPoint, this rectangle is guaranteed to contain the
 * specified point (among others), and all points in the rectangle are
 * plottable or not according to the value of clipresult.
 */
COORD clipminx;		/* minimum x value of cache rectangle */
COORD clipminy;		/* minimum y value of cache rectangle */
COORD clipmaxx;		/* maximum x value of cache rectangle */
COORD clipmaxy;		/* maximum y value of cache rectangle */

static BOOL	clipresult;	/* whether clip rectangle is plottable */
CLIPREGION *clipregion = NULL;

/*
 * Set an array of clip rectangles for future drawing actions.
 * Each pixel will be drawn only if lies in one or more of the specified
 * clip rectangles.  As a special case, specifying no rectangles implies
 * clipping is for the complete screen.  All clip rectangles are modified
 * if necessary to lie within the device area.  Call only after device
 * has been initialized.
 */
void
GdSetClipRects(int count, CLIPRECT *table)
{
  RECT	rc;

  if (clipregion)
	  GdDestroyClipRegion(clipregion);
  clipregion = GdAllocClipRegion();

  /* If there are no clip rectangles, then default to the full device area. */
  if (count <= 0) {
	clipminx = 0;
	clipminy = 0;
	clipmaxx = scrdev.xres - 1;
	clipmaxy = scrdev.yres - 1;
	clipresult = TRUE;
	return;
  }

  /* Copy the clip table to our own static array, modifying each
   * rectangle as necesary to fit within the device area.  If the clip
   * rectangle lies entirely outside of the device area, then skip it.
   */
  while (count-- > 0) {
	CLIPRECT cr;
	CLIPRECT *rp = &cr;

	*rp = *table++;
	if (rp->x < 0) {
		rp->width += rp->x;
		rp->x = 0;
	}
	if (rp->y < 0) {
		rp->height += rp->y;
		rp->y = 0;
	}
	if ((rp->x >= scrdev.xres) || (rp->width <= 0) ||
	    (rp->y >= scrdev.yres) || (rp->height <= 0))
		continue;
	if (rp->x + rp->width > scrdev.xres)
		rp->width = scrdev.xres - rp->x;
	if (rp->y + rp->height > scrdev.yres)
		rp->height = scrdev.yres - rp->y;
	rc.left = rp->x;
	rc.top = rp->y;
	rc.right = rp->x+rp->width;
	rc.bottom = rp->y+rp->height;
	GdUnionRectWithRegion(&rc, clipregion);
  }

  /* If there were no surviving clip rectangles, then set the clip
   * cache to prevent all drawing.
   */
  if (clipregion->numRects == 0) {
	clipminx = COORD_MIN;
	clipminy = COORD_MIN;
	clipmaxx = COORD_MAX;
	clipmaxy = COORD_MAX;
	clipresult = FALSE;
	return;
  }

  /* There was at least one valid clip rectangle. Default the clip
   * cache to be the first clip rectangle.
   */
  clipminx = clipregion->rects[0].left;
  clipminy = clipregion->rects[0].top;
  clipmaxx = clipregion->rects[0].right - 1;
  clipmaxy = clipregion->rects[0].bottom - 1;
  clipresult = TRUE;
}


/* Check a point against the list of clip rectangles.
 * Returns TRUE if the point is within one or more rectangles and thus
 * can be plotted, or FALSE if the point is not within any rectangle and
 * thus cannot be plotted.  Also remembers the coordinates of a clip cache
 * rectangle containing the specified point such that every point in the
 * rectangle would give the same result.  By examining this clip cache
 * rectangle after a call to this routine, the caller can efficiently
 * check many nearby points without needing any further calls.  If the
 * point lies within the cursor, then the cursor is removed.
 */
BOOL
GdClipPoint(COORD x,COORD y)
{
  int count;
  RECT *rp;
  COORD temp;

  /* First see whether the point lies within the current clip cache
   * rectangle.  If so, then we already know the result.
   */
  if ((x >= clipminx) && (x <= clipmaxx) &&
      (y >= clipminy) && (y <= clipmaxy)) {
	if (clipresult) GdCheckCursor(x, y, x, y);
	return clipresult;
  }

  /* If the point is outside of the screen area, then it is not
   * plottable, and the clip cache rectangle is the whole half-plane
   * outside of the screen area.
   */
  if (x < 0) {
	clipminx = COORD_MIN;
	clipmaxx = -1;
	clipminy = COORD_MIN;
	clipmaxy = COORD_MAX;
	clipresult = FALSE;
	return FALSE;
  }
  if (y < 0) {
	clipminx = COORD_MIN;
	clipmaxx = COORD_MAX;
	clipminy = COORD_MIN;
	clipmaxy = -1;
	clipresult = FALSE;
	return FALSE;
  }
  if (x >= scrdev.xres) {
	clipminx = scrdev.xres;
	clipmaxx = COORD_MAX;
	clipminy = COORD_MIN;
	clipmaxy = COORD_MAX;
	clipresult = FALSE;
	return FALSE;
  }
  if (y >= scrdev.yres) {
	clipminx = COORD_MIN;
	clipmaxx = COORD_MAX;
	clipminy = scrdev.yres;
	clipmaxy = COORD_MAX;
	clipresult = FALSE;
	return FALSE;
  }

  /* The point is within the screen area. If there are no clip
   * rectangles, then the point is plottable and the rectangle is the
   * whole screen.
   */
  count = clipregion->numRects;
  if (count <= 0) {
	clipminx = 0;
	clipmaxx = scrdev.xres - 1;
	clipminy = 0;
	clipmaxy = scrdev.yres - 1;
	clipresult = TRUE;
	GdCheckCursor(x, y, x, y);
	return TRUE;
  }

  /* We need to scan the list of clip rectangles to calculate a new
   * clip cache rectangle containing this point, and the result. First
   * see if the point lies within any of the clip rectangles. If so,
   * then it is plottable and use that clip rectangle as the cache
   * rectangle.  This is not necessarily the best result, but works ok
   * and is fast.
   */
  for (rp = clipregion->rects; count-- > 0; rp++) {
	if ((x >= rp->left) && (y >= rp->top) && (x < rp->right)
	    && (y < rp->bottom)) {
		clipminx = rp->left;
		clipminy = rp->top;
		clipmaxx = rp->right - 1;
		clipmaxy = rp->bottom - 1;
		clipresult = TRUE;
		GdCheckCursor(x, y, x, y);
		return TRUE;
	}
  }

  /* The point is not plottable. Scan the clip rectangles again to
   * determine a rectangle containing more non-plottable points.
   * Simply pick the largest rectangle whose area doesn't contain any
   * of the same coordinates as appropriate sides of the clip
   * rectangles.  This is not necessarily the best result, but works ok
   * and is fast.
   */
  clipminx = COORD_MIN;
  clipminy = COORD_MIN;
  clipmaxx = COORD_MAX;
  clipmaxy = COORD_MAX;
  count = clipregion->numRects;
  for (rp = clipregion->rects; count-- > 0; rp++) {
	if ((x < rp->left) && (rp->left <= clipmaxx)) clipmaxx = rp->left - 1;
	temp = rp->right - 1;
	if ((x > temp) && (temp >= clipminx)) clipminx = temp + 1;
	if ((y < rp->top) && (rp->top <= clipmaxy)) clipmaxy = rp->top - 1;
	temp = rp->bottom - 1;
	if ((y > temp) && (temp >= clipminy)) clipminy = temp + 1;
  }
  clipresult = FALSE;
  return FALSE;
}


/* Check the area determined by the specified pair of points against the
 * list of clip rectangles.  The area will either be totally visible,
 * totally visible, or possibly partially visible.  This routine updates
 * the clip cache rectangle, and returns one of the following values:
 *	CLIP_VISIBLE		The whole rectangle is visible
 *	CLIP_INVISIBLE		The whole rectangle is invisible
 *	CLIP_PARTIAL		The rectangle may be partially visible
 * In the case that the area is totally visible, the cursor is removed
 * if it overlaps the clip area.
 */
int
GdClipArea(COORD x1, COORD y1, COORD x2, COORD y2)
{
  if ((x1 < clipminx) || (x1 > clipmaxx) ||
      (y1 < clipminy) || (y1 > clipmaxy))
	GdClipPoint(x1, y1);

  if ((x2 >= clipminx) && (x2 <= clipmaxx) &&
      (y2 >= clipminy) && (y2 <= clipmaxy)) {
	if (!clipresult) return CLIP_INVISIBLE;
	GdCheckCursor(x1, y1, x2, y2);
	return CLIP_VISIBLE;
  }
  return CLIP_PARTIAL;
}
