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

#define BUF_SIZE 1024 

typedef	struct	chunk	CHUNK;
#define	CHUNKINITSIZE	4
struct	chunk	{
	CHUNK	*next;
	char	data[CHUNKINITSIZE];	/* actually of varying length */
};


static	CHUNK *	chunklist;



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
expandwildcards(name, maxargc, retargv)
	char	*name;
	int	maxargc;
	char	*retargv[];
{
	char	*last;
	char	*cp1, *cp2, *cp3;
	DIR	*dirp;
	struct	dirent	*dp;
	int	dirlen;
	int	matches;
	char	dirname[PATHLEN];

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
