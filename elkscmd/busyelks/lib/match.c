/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Utility routines.
 */

#include "../sash.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <utime.h>


/*
 * Routine to see if a text string is matched by a wildcard pattern.
 * Returns 1 if the text is matched, or 0 if it is not matched
 * or if the pattern is invalid.
 *  *		matches zero or more characters
 *  ?		matches a single character
 *  [abc]	matches 'a', 'b' or 'c'
 *  \c		quotes character c
 *  Adapted from code written by Ingo Wilken.
 */
int
match(text, pattern)
	char	*text;
	char	*pattern;
{
	char	*retrypat;
	char	*retrytxt;
	int	ch;
	int	found;

	retrypat = NULL;
	retrytxt = NULL;

	while (*text || *pattern) {
		ch = *pattern++;

		switch (ch) {
			case '*':  
				retrypat = pattern;
				retrytxt = text;
				break;

			case '[':  
				found = 0;
				while ((ch = *pattern++) != ']') {
					if (ch == '\\')
						ch = *pattern++;
					if (ch == '\0')
						return 0;
					if (*text == ch)
						found = 1;
				}
				if (!found) {
					pattern = retrypat;
					text = ++retrytxt;
				}
				/* fall into next case */

			case '?':  
				if (*text++ == '\0')
					return 0;
				break;

			case '\\':  
				ch = *pattern++;
				if (ch == '\0')
					return 0;
				/* fall into next case */

			default:        
				if (*text == ch) {
					if (*text)
						text++;
					break;
				}
				if (*text) {
					pattern = retrypat;
					text = ++retrytxt;
					break;
				}
				return 0;
		}

		if (pattern == NULL)
			return 0;
	}
	return 1;
}

