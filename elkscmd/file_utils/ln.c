/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Most simple built-in commands are here.
 */

#include "futils.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <utime.h>
#include <errno.h>

#define BUF_SIZE 1024 

typedef	struct	chunk	CHUNK;
#define	CHUNKINITSIZE	4
struct	chunk	{
	CHUNK	*next;
	char	data[CHUNKINITSIZE];	/* actually of varying length */
};

static	CHUNK *	chunklist;


/*
 * Return TRUE if a filename is a directory.
 * Nonexistant files return FALSE.
 */
BOOL isadir(char *name)
{
	struct stat statbuf;

	if (stat(name, &statbuf) < 0)
		return FALSE;

	return S_ISDIR(statbuf.st_mode);
}

/*
 * Build a path name from the specified directory name and file name.
 * If the directory name is NULL, then the original filename is returned.
 * The built path is in a static area, and is overwritten for each call.
 */
char *buildname(char *dirname, char *filename)
{
	char		*cp;
	static	char	buf[PATHLEN];

	if ((dirname == NULL) || (*dirname == '\0'))
		return filename;

	cp = strrchr(filename, '/');
	if (cp)
		filename = cp + 1;

	strcpy(buf, dirname);
	strcat(buf, "/");
	strcat(buf, filename);

	return buf;
}


int main(int argc, char **argv)
{
	int	dirflag;
	char	*srcname;
	char	*destname;
	char	*lastarg;

	if (argv[1][0] == '-') {
		if (strcmp(argv[1], "-s")) {
			write(STDERR_FILENO, "Unknown option\n", 15);
			exit(1);
		}

		if (argc != 4) {
			write(STDERR_FILENO, "Wrong number of arguments for symbolic link\n", 44);
			exit(1);
		}

		if (symlink(argv[2], argv[3]) < 0) {
			perror(argv[3]);
			exit(1);
		}
		exit(0);
	}

	/*
	 * Here for normal hard links.
	 */
	lastarg = argv[argc - 1];
	dirflag = isadir(lastarg);

	if ((argc > 3) && !dirflag) {
		write(STDERR_FILENO, lastarg, strlen(lastarg));
		write(STDERR_FILENO, ": not a directory\n", 18);
		exit(1);
	}

	while (argc-- > 2) {
		srcname = *(++argv);
		if (access(srcname, 0) < 0) {
			perror(srcname);
			continue;
		}

		destname = lastarg;
		if (dirflag)
			destname = buildname(destname, srcname);

		if (link(srcname, destname) < 0) {
			perror(destname);
			continue;
		}
	}
	exit(0);
}

