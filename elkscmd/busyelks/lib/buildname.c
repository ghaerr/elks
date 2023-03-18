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
 * Build a path name from the specified directory name and file name.
 * If the directory name is NULL, then the original filename is returned.
 * The built path is in a static area, and is overwritten for each call.
 */
char *buildname(char *dirname, char *filename)
{
	char *cp;
	static char buf[PATH_MAX];

	if ((dirname == NULL) || (*dirname == '\0')) return filename;

	cp = strrchr(filename, '/');
	if (cp) filename = cp + 1;

	strcpy(buf, dirname);
	strcat(buf, "/");
	strcat(buf, filename);

	return buf;
}
