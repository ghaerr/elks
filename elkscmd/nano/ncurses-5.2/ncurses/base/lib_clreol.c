/****************************************************************************
 * Copyright (c) 1998,1999,2000 Free Software Foundation, Inc.              *
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
**	lib_clreol.c
**
**	The routine wclrtoeol().
**
*/

#include <curses.priv.h>

MODULE_ID("$Id: lib_clreol.c,v 1.16 2000/04/29 21:14:54 tom Exp $")

int
wclrtoeol(WINDOW *win)
{
    int code = ERR;

    T((T_CALLED("wclrtoeol(%p)"), win));

    if (win) {
	chtype blank;
	chtype *ptr, *end;
	struct ldat *line;
	NCURSES_SIZE_T y = win->_cury;
	NCURSES_SIZE_T x = win->_curx;

	/*
	 * If we have just wrapped the cursor, the clear applies to the
	 * new line, unless we are at the lower right corner.
	 */
	if (win->_flags & _WRAPPED
	    && y < win->_maxy) {
	    win->_flags &= ~_WRAPPED;
	}

	/*
	 * There's no point in clearing if we're not on a legal
	 * position, either.
	 */
	if (win->_flags & _WRAPPED
	    || y > win->_maxy
	    || x > win->_maxx)
	    returnCode(ERR);

	blank = _nc_background(win);
	line = &win->_line[y];
	CHANGED_TO_EOL(line, x, win->_maxx);

	ptr = &(line->text[x]);
	end = &(line->text[win->_maxx]);

	while (ptr <= end)
	    *ptr++ = blank;

	_nc_synchook(win);
	code = OK;
    }
    returnCode(code);
}
