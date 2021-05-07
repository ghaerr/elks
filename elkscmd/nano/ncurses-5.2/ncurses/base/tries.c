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
**	tries.c
**
**	Functions to manage the tree of partial-completions for keycodes.
**
*/

#include <curses.priv.h>

MODULE_ID("$Id: tries.c,v 1.12 1999/03/01 23:23:59 tom Exp $")

/*
 * Expand a keycode into the string that it corresponds to, returning null if
 * no match was found, otherwise allocating a string of the result.
 */
char *_nc_expand_try(struct tries *tree, unsigned short code, int *count, size_t len)
{
	struct tries *ptr = tree;
	char *result = 0;

	if (code != 0) {
		while (ptr != 0) {
			if ((result = _nc_expand_try(ptr->child, code, count, len + 1)) != 0) {
				break;
			}
			if (ptr->value == code) {
				*count -= 1;
				if (*count == -1) {
					result = typeCalloc(char, len+2);
					break;
				}
			}
			ptr = ptr->sibling;
		}
	}
	if (result != 0) {
		if ((result[len] = ptr->ch) == 0)
			*((unsigned char *)(result+len)) = 128;
#ifdef TRACE
		if (len == 0)
			_tracef("expand_key %s %s", _trace_key(code), _nc_visbuf(result));
#endif
	}
	return result;
}

/*
 * Remove a code from the specified tree, freeing the unused nodes.  Returns
 * true if the code was found/removed.
 */
int _nc_remove_key(struct tries **tree, unsigned short code)
{
	T((T_CALLED("_nc_remove_key(%p,%d)"), tree, code));

	if (code == 0)
		returnCode(FALSE);
		
	while (*tree != 0) {
		if (_nc_remove_key(&(*tree)->child, code)) {
			returnCode(TRUE);
		}
		if ((*tree)->value == code) {
			if((*tree)->child) {
				/* don't cut the whole sub-tree */
				(*tree)->value = 0;
			} else {
				struct tries *to_free = *tree;
				*tree = (*tree)->sibling;
				free(to_free);
			}
			returnCode(TRUE);
		}
		tree = &(*tree)->sibling;
	}
	returnCode(FALSE);
}

/*
 * Remove a string from the specified tree, freeing the unused nodes.  Returns
 * true if the string was found/removed.
 */
int _nc_remove_string(struct tries **tree, char *string)
{
	T((T_CALLED("_nc_remove_string(%p,%s)"), tree, _nc_visbuf(string)));

	if (string == 0 || *string == 0)
		returnCode(FALSE);
		
	while (*tree != 0) {
		if ((unsigned char)(*tree)->ch == (unsigned char)*string) {
			if (string[1] != 0)
				returnCode(_nc_remove_string(&(*tree)->child, string+1));
			if((*tree)->child) {
				/* don't cut the whole sub-tree */
				(*tree)->value = 0;
			} else {
				struct tries *to_free = *tree;
				*tree = (*tree)->sibling;
				free(to_free);
			}
			returnCode(TRUE);
		}
		tree = &(*tree)->sibling;
	}
	returnCode(FALSE);
}
