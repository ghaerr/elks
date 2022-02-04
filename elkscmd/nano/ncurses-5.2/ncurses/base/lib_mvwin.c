/****************************************************************************
 * Copyright (c) 1998 Free Software Foundation, Inc.                        *
 *                                                                          *
 * Permission is hereby granted, free of charge, to any person obtaining a  *
 * copy of this software and associated documentation files (the            *
 * "Software"), to deal in the Software without restriction, including      *
 * without limitation the rights to use, copy, modify, merge, publish,      *
 * distribute, distribute with modifications, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is    *
 * furnished to do so, subject to the following conditions:                 *
 *                                                                          *
 * The above copyright notice and this permission notice shall be included  *
 * in all copies or substantial portions of the Software.                   *
 *                                                                          *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   *
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    *
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                          *
 * Except as contained in this notice, the name(s) of the above copyright   *
 * holders shall not be used in advertising or otherwise to promote the     *
 * sale, use or other dealings in this Software without prior written       *
 * authorization.                                                           *
 ****************************************************************************/

/****************************************************************************
 *  Author: Zeyd M. Ben-Halim <zmbenhal@netcom.com> 1992,1995               *
 *     and: Eric S. Raymond <esr@snark.thyrsus.com>                         *
 ****************************************************************************/



/*
**	lib_mvwin.c
**
**	The routine mvwin().
**
*/

#include <curses.priv.h>

MODULE_ID("$Id: lib_mvwin.c,v 1.7 1998/02/11 12:13:55 tom Exp $")

int mvwin(WINDOW *win, int by, int bx)
{
	T((T_CALLED("mvwin(%p,%d,%d)"), win, by, bx));

	if (!win || (win->_flags & _ISPAD))
	    returnCode(ERR);

	/* Copying subwindows is allowed, but it is expensive... */
	if (win->_flags & _SUBWIN) {
	  int err = ERR;
	  WINDOW *parent = win->_parent;
	  if (parent)
	    { /* Now comes the complicated and costly part, you should really
	       * try to avoid to move subwindows. Because a subwindow shares
	       * the text buffers with its parent, one can't do a simple
	       * memmove of the text buffers. One has to create a copy, then
	       * to relocate the subwindow and then to do a copy.
	       */
	      if ((by - parent->_begy == win->_pary) &&
		  (bx - parent->_begx == win->_parx))
		err=OK; /* we don't actually move */
	      else {
		WINDOW* clone = dupwin(win);  
		if (clone) {
		  /* now we have the clone, so relocate win */
		  
		  werase(win);             /* Erase the original place     */
		  wbkgd(win,parent->_bkgd);/* fill with parents background */
		  wsyncup(win);            /* Tell the parent(s)           */
		  
		  err = mvderwin(win,                   
				 by - parent->_begy,
				 bx - parent->_begx);
		  if (err!=ERR) {
		    err = copywin(clone,win,
				  0, 0, 0, 0, win->_maxy, win->_maxx, 0);
		    if (ERR!=err)
		      wsyncup(win);
		  }
		  if (ERR==delwin(clone))
		    err=ERR;
		}
	      }
	    }
	  returnCode(err);
	}

	if (by + win->_maxy > screen_lines - 1
	||  bx + win->_maxx > screen_columns - 1
	||  by < 0
	||  bx < 0)
	    returnCode(ERR);

	/*
	 * Whether or not the window is moved, touch the window's contents so
	 * that a following call to 'wrefresh()' will paint the window at the
	 * new location.  This ensures that if the caller has refreshed another
	 * window at the same location, that this one will be displayed.
	 */	
	win->_begy = by;
	win->_begx = bx;
	returnCode(touchwin(win));
}
