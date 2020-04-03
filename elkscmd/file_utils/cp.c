/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Most simple built-in commands are here.
 */

#include "futils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <utime.h>
#include <errno.h>

#define BUF_SIZE 4096

static char buf[BUF_SIZE];


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
	if (cp) filename = cp + 1;

	strcpy(buf, dirname);
	strcat(buf, "/");
	strcat(buf, filename);

	return buf;
}


/*
 * Return 1 if a filename is a directory.
 * Nonexistant files return 0.
 */
int isadir(char *name)
{
	struct	stat	statbuf;

	if (stat(name, &statbuf) < 0) return 0;
	return S_ISDIR(statbuf.st_mode);
}

/*
 * Copy one file to another, while possibly preserving its modes, times,
 * and modes.  Returns 1 if successful, or 0 on a failure with an
 * error message output.  (Failure is not indicated if the attributes cannot
 * be set.)
 */
int copyfile(char *srcname, char *destname, int setmodes)
{
	int		rfd;
	int		wfd;
	int		rcc;
	int		wcc;
	char		*bp;
	struct	stat	statbuf1;
	struct	stat	statbuf2;
	struct	utimbuf	times;

	if (stat(srcname, &statbuf1) < 0) {
		perror(srcname);
		return 0;
	}

	if (stat(destname, &statbuf2) < 0) {
		statbuf2.st_ino = -1;
		statbuf2.st_dev = -1;
	}

	if ((statbuf1.st_dev == statbuf2.st_dev) &&
		(statbuf1.st_ino == statbuf2.st_ino))
	{
		fprintf(stderr, "Copying file \"%s\" to itself\n", srcname);
		return 0;
	}

	rfd = open(srcname, 0);
	if (rfd < 0) {
		perror(srcname);
		return 0;
	}

	wfd = creat(destname, statbuf1.st_mode);
	if (wfd < 0) {
		perror(destname);
		close(rfd);
		return 0;
	}

	while ((rcc = read(rfd, buf, BUF_SIZE)) > 0) {
		bp = buf;
		while (rcc > 0) {
			wcc = write(wfd, bp, rcc);
			if (wcc < 0) {
				perror(destname);
				goto error_exit;
			}
			bp += wcc;
			rcc -= wcc;
		}
	}

	if (rcc < 0) {
		perror(srcname);
		goto error_exit;
	}

	close(rfd);
	if (close(wfd) < 0) {
		perror(destname);
		return 0;
	}

	if (setmodes) {
		(void) chmod(destname, statbuf1.st_mode);
		(void) chown(destname, statbuf1.st_uid, statbuf1.st_gid);

		times.actime = statbuf1.st_atime;
		times.modtime = statbuf1.st_mtime;

		(void) utime(destname, &times);
	}

	return 1;


error_exit:
	close(rfd);
	close(wfd);

	return 0;
}


int main(int argc, char **argv)
{
	int	dirflag;
	char	*srcname;
	char	*destname;
	char	*lastarg;

	if (argc < 3) goto usage;

	lastarg = argv[argc - 1];

	dirflag = isadir(lastarg);

	if ((argc > 3) && !dirflag) {
		fprintf(stderr, "%s: not a directory\n", lastarg);
		return 1;
	}

	while (argc-- > 2) {
		srcname = argv[1];
		destname = lastarg;
		if (dirflag) destname = buildname(destname, srcname);

		if (!copyfile(*++argv, destname, 0)) goto error_copy;
	}
	return 0;

error_copy:
	fprintf(stderr, "Failed to copy %s -> %s\n", srcname, destname);
	return 1;
usage:
	fprintf(stderr, "usage: %s source_file dest_file\n", argv[0]);
	fprintf(stderr, "       %s file1 file2 ... dest_directory\n", argv[0]);
	return 1;
}
