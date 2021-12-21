/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Wildcard name expansion, borrowed from sash.
 */

#include <regex.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define FALSE	0
#define TRUE	1

typedef	struct	chunk	CHUNK;
#define	CHUNKINITSIZE	4
struct	chunk	{
	CHUNK	*next;
	char	data[CHUNKINITSIZE];	/* actually of varying length */
};

static	CHUNK *	chunklist;

/*
 * Allocate a chunk of memory (like malloc).
 * The difference, though, is that the memory allocated is put on a
 * list of chunks which can be freed all at one time.  You CAN NOT free
 * an individual chunk.
 */
static char *
getchunk(int size)
{
	CHUNK	*chunk;

	if (size < CHUNKINITSIZE)
		size = CHUNKINITSIZE;

	chunk = (CHUNK *) malloc(size + sizeof(CHUNK) - CHUNKINITSIZE);
	if (chunk == NULL)
		return NULL;

	chunk->next = chunklist;
	chunklist = chunk;

	return chunk->data;
}


/*
 * Free all chunks of memory that had been allocated since the last
 * call to this routine.
 */
void
freewildcards(void)
{
	CHUNK	*chunk;

	while (chunklist) {
		chunk = chunklist;
		chunklist = chunk->next;
		free((char *) chunk);
	}
}

/*
 * Routine to see if a text string is matched by a wildcard pattern.
 * Returns TRUE if the text is matched, or FALSE if it is not matched
 * or if the pattern is invalid.
 *  *		matches zero or more characters
 *  ?		matches a single character
 *  [abc]	matches 'a', 'b' or 'c'
 *  \c		quotes character c
 *  Adapted from code written by Ingo Wilken.
 */
static int
match(char *text, char *pattern)
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
				found = FALSE;
				while ((ch = *pattern++) != ']') {
					if (ch == '\\')
						ch = *pattern++;
					if (ch == '\0')
						return FALSE;
					if (*text == ch)
						found = TRUE;
				}
				if (!found) {
					pattern = retrypat;
					text = ++retrytxt;
				}
				/* fall into next case */

			case '?':
				if (*text++ == '\0')
					return FALSE;
				break;

			case '\\':
				ch = *pattern++;
				if (ch == '\0')
					return FALSE;
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
				return FALSE;
		}

		if (pattern == NULL)
			return FALSE;
	}
	return TRUE;
}

/*
 * Sort routine for list of filenames.
 */
static int
namesort(char **p1, char **p2)
{
	return strcmp(*p2, *p1);
}

/*
 * Expand the wildcards in a filename, if any.
 * Returns an argument list with matching filenames in sorted order.
 * The expanded names are stored in memory chunks which can later all
 * be freed at once.  Returns zero if the name is not a wildcard, or
 * returns the count of matched files if the name is a wildcard and
 * there was at least one match, or returns -1 if either no filenames
 * matched or too many filenames matched (with an error output).
 */
int
expandwildcards(char *name, int maxargc, char **retargv)
{
	char	*last;
	char	*cp1, *cp2, *cp3;
	DIR	*dirp;
	struct	dirent	*dp;
	int	dirlen;
	int	matches;
	char	dirname[256];

	last = strrchr(name, '/');
	if (last)
		last++;
	else
		last = name;

	cp1 = strchr(name, '*');
	cp2 = strchr(name, '?');
	cp3 = strchr(name, '[');

	if ((cp1 == NULL) && (cp2 == NULL) && (cp3 == NULL))
		return 0;

	if ((cp1 && (cp1 < last)) || (cp2 && (cp2 < last)) ||
		(cp3 && (cp3 < last)))
	{
		fprintf(stderr, "Wildcards only implemented for last filename component\n");
		return -1;
	}

	dirname[0] = '.';
	dirname[1] = '\0';

	if (last != name) {
		memcpy(dirname, name, last - name);
		dirname[last - name - 1] = '\0';
		if (dirname[0] == '\0') {
			dirname[0] = '/';
			dirname[1] = '\0';
		}
	}

	dirp = opendir(dirname);
	if (dirp == NULL) {
		perror(dirname);
		return -1;
	}

	dirlen = strlen(dirname);
	if (last == name) {
		dirlen = 0;
		dirname[0] = '\0';
	} else if (dirname[dirlen - 1] != '/') {
		dirname[dirlen++] = '/';
		dirname[dirlen] = '\0';
	}

	matches = 0;

	while ((dp = readdir(dirp)) != NULL) {
		if ((strcmp(dp->d_name, ".") == 0) ||
			(strcmp(dp->d_name, "..") == 0))
				continue;

		if (!match(dp->d_name, last))
			continue;

		if (matches >= maxargc) {
			fprintf(stderr, "Too many filename matches\n");
			closedir(dirp);
			return -1;
		}

		cp1 = getchunk(dirlen + strlen(dp->d_name) + 1);
		if (cp1 == NULL) {
			fprintf(stderr, "No memory for filename\n");
			closedir(dirp);
			return -1;
		}

		if (dirlen)
			memcpy(cp1, dirname, dirlen);
		strcpy(cp1 + dirlen, dp->d_name);

		retargv[matches++] = cp1;
	}

	closedir(dirp);

	if (matches == 0) {
		fprintf(stderr, "No matches\n");
		return -1;
	}

	qsort((char *) retargv, matches, sizeof(char *), namesort);

	return matches;
}
