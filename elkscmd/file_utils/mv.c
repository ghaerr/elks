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


/*
 * Return 1 if a filename is a directory.
 * Nonexistant files return 0.
 */
int isadir(name)
	char	*name;
{
	struct	stat	statbuf;

	if (stat(name, &statbuf) < 0)
		return 0;

	return S_ISDIR(statbuf.st_mode);
}

/*
 * Copy one file to another, while possibly preserving its modes, times,
 * and modes.  Returns 1 if successful, or 0 on a failure with an
 * error message output.  (Failure is not indicted if the attributes cannot
 * be set.)
 */
int copyfile(srcname, destname, setmodes)
	char	*srcname;
	char	*destname;
	int	setmodes;
{
	int		rfd;
	int		wfd;
	int		rcc;
	int		wcc;
	char		*bp;
	static char buf[BUF_SIZE];
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
		write(STDERR_FILENO, "Copying file \"", 14);
		write(STDERR_FILENO, srcname, strlen(srcname));
		write(STDERR_FILENO, "\" to itself\n", 12);
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

	if (argc < 3) goto usage;

	lastarg = argv[argc - 1];

	dirflag = isadir(lastarg);

	if ((argc > 3) && !dirflag) {
		fprintf(stderr, "%s: not a directory\n", lastarg);
		goto usage;
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

		if (rename(srcname, destname) >= 0)
			continue;

		if (errno != EXDEV) {
			perror(destname);
			continue;
		}

		if (!copyfile(srcname, destname, 1))
			continue;

		if (unlink(srcname) < 0)
			perror(srcname);
	}
	return 0;

usage:
	fprintf(stderr, "usage: %s source_file dest_file\n", argv[0]);
	fprintf(stderr, "       %s file1 [file2] ... dest_dir\n", argv[0]);
	return 1;
}
