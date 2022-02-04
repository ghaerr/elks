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
**	lib_scanw.c
**
**	The routines scanw(), wscanw() and friends.
**
*/

#include <curses.priv.h>

MODULE_ID("$Id: lib_scanw.c,v 1.8 1998/04/11 22:54:18 tom Exp $")

#if !HAVE_VSSCANF
#if defined(__QNX__)
extern int vsscanf(const char *str, const char *format, __va_list __arg);
#else
extern int vsscanf(const char *str, const char *format, ...);
#endif
#endif

int vwscanw(WINDOW *win, NCURSES_CONST char *fmt, va_list argp)
{
char buf[BUFSIZ];

	if (wgetnstr(win, buf, sizeof(buf)-1) == ERR)
	    return(ERR);

	return(vsscanf(buf, fmt, argp));
}

int scanw(NCURSES_CONST char *fmt, ...)
{
int code;
va_list ap;

	T(("scanw(\"%s\",...) called", fmt));

	va_start(ap, fmt);
	code = vwscanw(stdscr, fmt, ap);
	va_end(ap);
	return (code);
}

int wscanw(WINDOW *win, NCURSES_CONST char *fmt, ...)
{
int code;
va_list ap;

	T(("wscanw(%p,\"%s\",...) called", win, fmt));

	va_start(ap, fmt);
	code = vwscanw(win, fmt, ap);
	va_end(ap);
	return (code);
}

int mvscanw(int y, int x, NCURSES_CONST char *fmt, ...)
{
int code;
va_list ap;

	va_start(ap, fmt);
	code = (move(y, x) == OK) ? vwscanw(stdscr, fmt, ap) : ERR;
	va_end(ap);
	return (code);
}

int mvwscanw(WINDOW *win, int y, int x, NCURSES_CONST char *fmt, ...)
{
int code;
va_list ap;

	va_start(ap, fmt);
	code = (wmove(win, y, x) == OK) ? vwscanw(win, fmt, ap) : ERR;
	va_end(ap);
	return (code);
}
