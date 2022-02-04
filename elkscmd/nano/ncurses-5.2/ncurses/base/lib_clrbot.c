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
**	lib_clrbot.c
**
**	The routine wclrtobot().
**
*/

#include <curses.priv.h>

MODULE_ID("$Id: lib_clrbot.c,v 1.15 2000/04/29 21:15:26 tom Exp $")

int
wclrtobot(WINDOW *win)
{
    int code = ERR;

    T((T_CALLED("wclrtobot(%p)"), win));

    if (win) {
	NCURSES_SIZE_T y;
	NCURSES_SIZE_T startx = win->_curx;
	chtype blank = _nc_background(win);

	T(("clearing from y = %d to y = %d with maxx =  %d",
		win->_cury, win->_maxy, win->_maxx));

	for (y = win->_cury; y <= win->_maxy; y++) {
	    struct ldat *line = &(win->_line[y]);
	    chtype *ptr = &(line->text[startx]);
	    chtype *end = &(line->text[win->_maxx]);

	    CHANGED_TO_EOL(line, startx, win->_maxx);

	    while (ptr <= end)
		*ptr++ = blank;

	    startx = 0;
	}
	_nc_synchook(win);
	code = OK;
    }
    returnCode(code);
}
