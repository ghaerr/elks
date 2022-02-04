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
#include <dirent.h>

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

/*
 * Link all files in source directory to same name in destination directory.
 * Returns 1 if successful, or 0 on a failure with an * error message output.
 */
int linkfiles(char *srcdir, char *destdir)
{
	DIR *dirp;
	struct dirent *dp;
	char *newsrc;
	char newdest[PATHLEN];

	dirp = opendir(srcdir);
	if (!dirp) {
		perror(srcdir);
		return 0;
	}

	/* pre-search directory looking for subdirectories or symlinks */
	while ((dp = readdir(dirp)) != NULL) {
		struct stat sbuf;

		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;

		newsrc = buildname(srcdir, dp->d_name);
		if (lstat(newsrc, &sbuf) >= 0 && (S_ISDIR(sbuf.st_mode) || S_ISLNK(sbuf.st_mode))) {
			fprintf(stderr, "Can't move directory or symlink: %s\n", newsrc);
			closedir(dirp);
			return 0;
		}
	}
	rewinddir(dirp);

	/* now link each file to new directory*/
	while ((dp = readdir(dirp)) != NULL) {
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;

		strcpy(newdest, buildname(destdir, dp->d_name));
		newsrc = buildname(srcdir, dp->d_name);

		/* link will fail if directory or symlink*/
		if (link(newsrc, newdest) < 0) {
			perror(newsrc);
			return 0;
		}

		if (unlink(newsrc) < 0) {
			perror(newsrc);
			return 0;
		}
	}
	closedir(dirp);
	return 1;
}

/*
 * Copy one file to another, while possibly preserving its modes, times,
 * and modes.  Returns 0 if successful, or 1 on a failure with an
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
		return 1;
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
		return 1;
	}

	rfd = open(srcname, 0);
	if (rfd < 0) {
		perror(srcname);
		return 1;
	}

	wfd = creat(destname, statbuf1.st_mode);
	if (wfd < 0) {
		perror(destname);
		close(rfd);
		return 1;
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
	close(wfd);

	if (setmodes) {
		(void) chmod(destname, statbuf1.st_mode);

		(void) chown(destname, statbuf1.st_uid, statbuf1.st_gid);

		times.actime = statbuf1.st_atime;
		times.modtime = statbuf1.st_mtime;

		(void) utime(destname, &times);
	}

	return 0;


error_exit:
	close(rfd);
	close(wfd);

	return 1;
}


int main(int argc, char **argv)
{
	int	dirflag;
	char	*srcname;
	char	*destname;
	char	*lastarg;
	struct stat sbuf;

	if (argc < 3) goto usage;

	lastarg = argv[argc - 1];

	dirflag = isadir(lastarg);

	if ((argc > 3) && !dirflag) {
		fprintf(stderr, "%s: not a directory\n", lastarg);
		goto usage;
	}

	while (argc-- > 2) {
		srcname = *(++argv);

		destname = lastarg;
		if (dirflag)
			destname = buildname(destname, srcname);

		/* handle renaming symlinks*/
		if (lstat(srcname, &sbuf) >= 0 && S_ISLNK(sbuf.st_mode)) {
			char buf[PATHLEN];
			int len = readlink(srcname, buf, PATHLEN - 1);
			if (len < 0) {
				perror(srcname);
				continue;
			}
			buf[len] = '\0';
			if (!dirflag && access(destname, F_OK) == 0)
				if (unlink(destname) < 0)
					perror(destname);
			if (symlink(buf, destname) < 0) {
				perror(destname);
				continue;
			}
			if (unlink(srcname) < 0)
				perror(srcname);
			continue;
		}

		if (access(srcname, F_OK) < 0) {
			perror(srcname);
			continue;
		}

		if (!dirflag) {
			/* remove destname if exists and not a directory*/
			if (access(destname, F_OK) == 0 && !isadir(destname))
				if (unlink(destname) < 0)
					perror(destname);
		}

		if (rename(srcname, destname) >= 0)
			continue;

		/* handle broken kernel directory rename (issue #583)*/
		if (errno == EPERM && access(destname, F_OK) < 0 && isadir(srcname)) {
			char destdir[PATHLEN];

			if (mkdir(destname, 0777 & ~umask(0))) {
				perror(destname);
				return 1;
			}
			strcpy(destdir, destname);

			/* only works if source directory has no subdirectories or symlinks!*/
			if (!linkfiles(srcname, destdir)) {
				rmdir(destdir);	/* remove directory just created*/
				return 1;
			}

			if (rmdir(srcname) < 0) {
				perror(srcname);
				return 1;
			}
			continue;
		}

		if (errno != EXDEV) {
			perror(destname);
			continue;
		}

		if (copyfile(srcname, destname, 1))
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
