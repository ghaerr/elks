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
 *  Author: Thomas E. Dickey <dickey@clark.net> 1997                        *
 ****************************************************************************/
/*
 *	trace_buf.c - Tracing/Debugging buffers (attributes)
 */

#include <curses.priv.h>

MODULE_ID("$Id: trace_buf.c,v 1.7 1999/02/27 19:50:58 tom Exp $")

typedef struct {
	char *text;
	size_t size;
} LIST;

char * _nc_trace_buf(int bufnum, size_t want)
{
	static LIST *list;
	static size_t have;

#if NO_LEAKS
	if (bufnum < 0) {
		if (have) {
			while (have--) {
				free(list[have].text);
			}
			free(list);
		}
		return 0;
	}
#endif

	if ((size_t)(bufnum+1) > have) {
		size_t need = (bufnum + 1) * 2;
		if ((list = typeRealloc(LIST, need, list)) == 0)
			return(0);
		while (need > have)
			list[have++].text = 0;
	}

	if (list[bufnum].text == 0
	 || want > list[bufnum].size)
	{
		if ((list[bufnum].text = typeRealloc(char, want, list[bufnum].text)) != 0)
			list[bufnum].size = want;
	}

	if (list[bufnum].text != 0)
		*(list[bufnum].text) = '\0';
	return list[bufnum].text;
}
