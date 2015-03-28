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


void do_exit() {
	exit(0);
}

/*
 * Take a command string, and break it up into an argc, argv list.
 * The returned argument list and strings are in static memory, and so
 * are overwritten on each call.  The argument array is ended with an
 * extra NULL pointer for convenience.  Returns TRUE if successful,
 * or FALSE on an error with a message already output.
 */
BOOL
makeargs(cmd, argcptr, argvptr)
	char	*cmd;
	int	*argcptr;
	char	***argvptr;
{
	char		*cp;
	int		argc;
	static char	strings[CMDLEN+1];
	static char	*argtable[MAXARGS+1];

	/*
	 * Copy the command string and then break it apart
	 * into separate arguments.
	 */
	strcpy(strings, cmd);
	argc = 0;
	cp = strings;

	while (*cp) {
		if (argc >= MAXARGS) {
			fprintf(stderr, "Too many arguments\n");
			return FALSE;
		}

		argtable[argc++] = cp;

		while (*cp && !isblank(*cp))
			cp++;

		while (isblank(*cp))
 			*cp++ = '\0';
	}

	argtable[argc] = NULL;

	*argcptr = argc;
	*argvptr = argtable;

 	return TRUE;
}

/*
 * Make a NULL-terminated string out of an argc, argv pair.
 * Returns TRUE if successful, or FALSE if the string is too long,
 * with an error message given.  This does not handle spaces within
 * arguments correctly.
 */
BOOL
makestring(argc, argv, buf, buflen)
	char	**argv;
	char	*buf;
{
	int	len;

	while (argc-- > 0) {
		len = strlen(*argv);
		if (len >= buflen) {
			fprintf(stderr, "Argument string too long\n");
			return FALSE;
		}

		strcpy(buf, *argv++);

		buf += len;
		buflen -= len;

		if (argc)
			*buf++ = ' ';
		buflen--; 
	}

	*buf = '\0';

	return TRUE;
}


/* END CODE */
