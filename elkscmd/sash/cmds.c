/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Most simple built-in commands are here.
 */

#include "sash.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <utime.h>
#include <errno.h>


#ifdef CMD_ECHO
void
do_echo(argc, argv)
	char	**argv;
{
	BOOL	first;

	first = TRUE;
	while (argc-- > 1) {
		if (!first)
			fputc(' ', stdout);
		first = FALSE;
		fputs(*++argv, stdout);
	}
	fputc('\n', stdout);
}
#endif /* CMD_ECHO */

#ifdef CMD_PWD
void
do_pwd(argc, argv)
	char	**argv;
{
	char	buf[PATHLEN];

	if (getcwd(buf, PATHLEN) == NULL) {
		fprintf(stderr, "Cannot get current directory\n");
		return;
	}

	printf("%s\n", buf);
	fflush(stdout);
}
#endif /* CMD_PWD */

void
do_cd(argc, argv)
	char	**argv;
{
	char	*path;

	if (argc > 1)
		path = argv[1];
	else {
		path = getenv("HOME");
		if (path == NULL) {
			fprintf(stderr, "No HOME environment variable\n");
			return;
		}
	}

	if (chdir(path) < 0)
		perror(path);
}

#ifdef CMD_MKDIR
void
do_mkdir(argc, argv)
	char	**argv;
{
	while (argc-- > 1) {
		if (mkdir(argv[1], 0777) < 0)
			perror(argv[1]);
		argv++;
	}
}
#endif /* CMD_MKDIR */

#ifdef CMD_MKNOD
void
do_mknod(argc, argv)
	char	**argv;
{
	char	*cp;
	int	mode;
	int	major;
	int	minor;

	mode = 0666;

	if (strcmp(argv[2], "b") == 0)
		mode |= S_IFBLK;
	else if (strcmp(argv[2], "c") == 0)
		mode |= S_IFCHR;
	else {
		fprintf(stderr, "Bad device type\n");
		return;
	}

	major = 0;
	cp = argv[3];
	while (isdecimal(*cp))
		major = major * 10 + *cp++ - '0';

	if (*cp || (major < 0) || (major > 255)) {
		fprintf(stderr, "Bad major number\n");
		return;
	}

	minor = 0;
	cp = argv[4];
	while (isdecimal(*cp))
		minor = minor * 10 + *cp++ - '0';

	if (*cp || (minor < 0) || (minor > 255)) {
		fprintf(stderr, "Bad minor number\n");
		return;
	}

	if (mknod(argv[1], mode, major * 256 + minor) < 0)
		perror(argv[1]);
}
#endif /* CMD_MKNOD */

#ifdef CMD_RMDIR
void
do_rmdir(argc, argv)
	char	**argv;
{
	while (argc-- > 1) {
		if (rmdir(argv[1]) < 0)
			perror(argv[1]);
		argv++;
	}
}
#endif /* CMD_RMDIR */

#ifdef CMD_SYNC
void
do_sync(argc, argv)
	char	**argv;
{
	sync();
}
#endif /* CMD_SYNC */


#ifdef CMD_RM
void
do_rm(argc, argv)
	char	**argv;
{
	struct stat sbuf;

	while (argc-- > 1) {
		if (!lstat(argv[1], &sbuf)) {
			if (unlink(argv[1]) < 0)
				fprintf(stderr, "Could not remove %s\n", argv[1]);
		} else fprintf(stderr, "%s not found\n", argv[1]);
		argv++;
	}
}
#endif /* CMD_RM */

#ifdef CMD_CHMOD
void
do_chmod(argc, argv)
	char	**argv;
{
	char	*cp;
	int	mode;

	mode = 0;
	cp = argv[1];
	while (isoctal(*cp))
		mode = mode * 8 + (*cp++ - '0');

	if (*cp) {
		fprintf(stderr, "Mode must be octal\n");
		return;
	}
	argc--;
	argv++;

	while (argc-- > 1) {
		if (chmod(argv[1], mode) < 0)
			perror(argv[1]);
		argv++;
	}
}
#endif /* CMD_CHMOD */


#ifdef CMD_CHOWN
void
do_chown(argc, argv)
	char	**argv;
{
	char		*cp;
	int		uid;
	struct passwd	*pwd;
	struct stat	statbuf;

	cp = argv[1];
	if (isdecimal(*cp)) {
		uid = 0;
		while (isdecimal(*cp))
			uid = uid * 10 + (*cp++ - '0');

		if (*cp) {
			fprintf(stderr, "Bad uid value\n");
			return;
		}
	} else {
		pwd = getpwnam(cp);
		if (pwd == NULL) {
			fprintf(stderr, "Unknown user name\n");
			return;
		}

		uid = pwd->pw_uid;
	}

	argc--;
	argv++;

	while (argc-- > 1) {
		argv++;
		if ((stat(*argv, &statbuf) < 0) ||
			(chown(*argv, uid, statbuf.st_gid) < 0))
				perror(*argv);
	}
}
#endif /* CMD_CHOWN */

#ifdef CMD_CHGRP
void
do_chgrp(argc, argv)
	char	**argv;
{
	char		*cp;
	int		gid;
	struct group	*grp;
	struct stat	statbuf;

	cp = argv[1];
	if (isdecimal(*cp)) {
		gid = 0;
		while (isdecimal(*cp))
			gid = gid * 10 + (*cp++ - '0');

		if (*cp) {
			fprintf(stderr, "Bad gid value\n");
			return;
		}
	} else {
		grp = getgrnam(cp);
		if (grp == NULL) {
			fprintf(stderr, "Unknown group name\n");
			return;
		}

		gid = grp->gr_gid;
	}

	argc--;
	argv++;

	while (argc-- > 1) {
		argv++;
		if ((stat(*argv, &statbuf) < 0) ||
			(chown(*argv, statbuf.st_uid, gid) < 0))
				perror(*argv);
	}
}
#endif /* CMD_CHGRP */

#ifdef CMD_TOUCH
void
do_touch(argc, argv)
	char	**argv;
{
	char		*name;
	int		fd;
	struct	utimbuf	now;

	time(&now.actime);
	now.modtime = now.actime;

	while (argc-- > 1) {
		name = *(++argv);

		fd = open(name, O_CREAT | O_WRONLY | O_EXCL, 0666);
		if (fd >= 0) {
			close(fd);
			continue;
		}

		if (utime(name, &now) < 0)
			perror(name);
	}
}
#endif /* CMD_TOUCH */

#ifdef CMD_MV
void
do_mv(argc, argv)
	char	**argv;
{
	int	dirflag;
	char	*srcname;
	char	*destname;
	char	*lastarg;

	lastarg = argv[argc - 1];

	dirflag = isadir(lastarg);

	if ((argc > 3) && !dirflag) {
		fprintf(stderr, "%s: not a directory\n", lastarg);
		return;
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

		if (!copyfile(srcname, destname, TRUE))
			continue;

		if (unlink(srcname) < 0)
			perror(srcname);
	}
}
#endif /* CMD_MV */

#ifdef CMD_LN
void
do_ln(argc, argv)
	char	**argv;
{
	int	dirflag;
	char	*srcname;
	char	*destname;
	char	*lastarg;

	if (argv[1][0] == '-') {
		if (strcmp(argv[1], "-s")) {
			fprintf(stderr, "Unknown option\n");
			return;
		}

		if (argc != 4) {
			fprintf(stderr, "Wrong number of arguments for symbolic link\n");
			return;
		}

#ifdef	S_ISLNK
		if (symlink(argv[2], argv[3]) < 0)
			perror(argv[3]);
#else
		fprintf(stderr, "Symbolic links are not allowed\n");
#endif
		return;
	}

	/*
	 * Here for normal hard links.
	 */
	lastarg = argv[argc - 1];
	dirflag = isadir(lastarg);

	if ((argc > 3) && !dirflag) {
		fprintf(stderr, "%s: not a directory\n", lastarg);
		return;
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
}
#endif /* CMD_LN */

#ifdef CMD_CP
void
do_cp(argc, argv)
	char	**argv;
{
	BOOL	dirflag;
	char	*srcname;
	char	*destname;
	char	*lastarg;

	lastarg = argv[argc - 1];

	dirflag = isadir(lastarg);

	if ((argc > 3) && !dirflag) {
		fprintf(stderr, "%s: not a directory\n", lastarg);
		return;
	}

	while (argc-- > 2) {
		srcname = argv[1];
		destname = lastarg;
		if (dirflag)
			destname = buildname(destname, srcname);

		(void) copyfile(*++argv, destname, FALSE);
	}
}
#endif /* CMD_CP */

#ifdef CMD_MOUNT
void
do_mount(int argc, char **argv)
{
	char	*str;
	int		type = 0;		/* default fs*/
	int		flags = 0;
	char	*option;

	argc--;
	argv++;

	while ((argc > 0) && (**argv == '-')) {
		argc--;
		str = *argv++ ;

		while (*++str) switch (*str) {
			case 't':
				if ((argc <= 0) || (**argv == '-')) {
					fprintf(stderr, "mount: missing file system type\n");
					return;
				}

				option = *argv++;
				if (!strcmp(option, "minix"))
					type = FST_MINIX;
				else if (!strcmp(option, "msdos") || !strcmp(option, "fat"))
					type = FST_MSDOS;
				else if (!strcmp(option, "romfs"))
					type = FST_ROMFS;
				argc--;
				break;

			case 'o':
				if ((argc <= 0) || (**argv == '-')) {
					fprintf(stderr, "mount: missing option string\n");
					return;
				}

				option = *argv++;
				if (!strcmp(option, "ro"))
					flags = MS_RDONLY;
				else if (!strcmp(option, "remount,rw"))
					flags = MS_REMOUNT;
				else if (!strcmp(option, "remount,ro"))
					flags = MS_REMOUNT|MS_RDONLY;
				else {
					fprintf(stderr, "mount: bad option string\n");
					return;
				}
				argc--;
				break;

			case 'a':
				flags |= MS_AUTOMOUNT;
				break;

			default:
				fprintf(stderr, "mount: unknown option\n");
				return;
		}
	}

	if (argc != 2) {
		fprintf(stderr, "Usage: mount [-t type] [-o ro|remount,rw] <device> <directory>\n");
		return;
	}

	if (mount(argv[0], argv[1], type, flags) < 0) {
		if (flags & MS_AUTOMOUNT) {
			type = (!type || type == FST_MINIX)? FST_MSDOS: FST_MINIX;
			if (mount(argv[0], argv[1], type, flags) < 0) {
				perror("mount failed");
				return;
			}
		}
	}
}
#endif /* CMD_MOUNT */

#ifdef CMD_MOUNT
void
do_umount(argc, argv)
	char	**argv;
{
	if (argc != 2) {
		fprintf(stderr, "Usage: umount <device>|<directory>\n");
		return;
	}
	if (umount(argv[1]) < 0)
		perror(argv[1]);
}
#endif /* CMD_MOUNT */

#ifdef CMD_CMP
void
do_cmp(argc, argv)
	char	**argv;
{
	int		fd1;
	int		fd2;
	int		cc1;
	int		cc2;
	long		pos;
	char		*bp1;
	char		*bp2;
	char		buf1[512];
	char		buf2[512];
	struct	stat	statbuf1;
	struct	stat	statbuf2;

	if (stat(argv[1], &statbuf1) < 0) {
		perror(argv[1]);
		return;
	}

	if (stat(argv[2], &statbuf2) < 0) {
		perror(argv[2]);
		return;
	}

	if ((statbuf1.st_dev == statbuf2.st_dev) &&
		(statbuf1.st_ino == statbuf2.st_ino))
	{
		printf("Files are links to each other\n");
		return;
	}

	if (statbuf1.st_size != statbuf2.st_size) {
		printf("Files are different sizes\n");
		return;
	}

	fd1 = open(argv[1], 0);
	if (fd1 < 0) {
		perror(argv[1]);
		return;
	}

	fd2 = open(argv[2], 0);
	if (fd2 < 0) {
		perror(argv[2]);
		close(fd1);
		return;
	}

	pos = 0;
	while (TRUE) {
		if (intflag)
			goto closefiles;

		cc1 = read(fd1, buf1, sizeof(buf1));
		if (cc1 < 0) {
			perror(argv[1]);
			goto closefiles;
		}

		cc2 = read(fd2, buf2, sizeof(buf2));
		if (cc2 < 0) {
			perror(argv[2]);
			goto closefiles;
		}

		if ((cc1 == 0) && (cc2 == 0)) {
			printf("Files are identical\n");
			goto closefiles;
		}

		if (cc1 < cc2) {
			printf("First file is shorter than second\n");
			goto closefiles;
		}

		if (cc1 > cc2) {
			printf("Second file is shorter than first\n");
			goto closefiles;
		}

		if (memcmp(buf1, buf2, cc1) == 0) {
			pos += cc1;
			continue;
		}

		bp1 = buf1;
		bp2 = buf2;
		while (*bp1++ == *bp2++)
			pos++;

		printf("Files differ at byte position %ld\n", pos);
		goto closefiles;
	}

closefiles:
	close(fd1);
	close(fd2);
}
#endif /* CMD_CMP */

#ifdef CMD_MORE
void
do_more(argc, argv)
	char	**argv;
{
	FILE	*fp;
	char	*name;
	int	ch;
	int	line;
	int	col;
	char	buf[80];

	while (argc-- > 1) {
		name = *(++argv);

		fp = fopen(name, "r");
		if (fp == NULL) {
			perror(name);
			return;
		}

		/*printf("<< %s >>\n", name);*/
		line = 1;
		col = 0;

		while (fp && ((ch = fgetc(fp)) != EOF)) {
			switch (ch) {
				case '\r':
					col = 0;
					break;

				case '\n':
					line++;
					col = 0;
					break;

				case '\t':
					col = ((col + 1) | 0x07) + 1;
					break;

				case '\b':
					if (col > 0)
						col--;
					break;

				default:
					col++;
			}

			putchar(ch);
			if (col >= 80) {
				col -= 80;
				line++;
			}

			if (line < 24)
				continue;

			if (col > 0)
				putchar('\n');

			printf("--More--");
			fflush(stdout);

			if (intflag || (read(0, buf, sizeof(buf)) < 0)) {
				if (fp)
					fclose(fp);
				return;
			}

			ch = buf[0];
			if (ch == ':')
				ch = buf[1];

			switch (ch) {
				case 'N':
				case 'n':
					fclose(fp);
					fp = NULL;
					break;

				case 'Q':
				case 'q':
					fclose(fp);
					return;
			}

			col = 0;
			line = 1;
		}
		if (fp)
			fclose(fp);
	}
}
#endif /* CMD_MORE */


void
do_exit(argc, argv)
	char	**argv;
{
	exit(0);
}

#ifdef CMD_SETENV
void
do_setenv(argc, argv)
	char	**argv;
{
	char	buf[CMDLEN];

	strcpy(buf, argv[1]);
	if (argc > 2) {
		strcat(buf, "=");
		strcat(buf, argv[2]);
	}
	if (putenv(buf)) 
		perror("setenv");

}
#endif /* CMD_SETENV */

#ifdef CMD_PRINTENV
void
do_printenv(argc, argv)
	char	**argv;
{
	char		**env = environ;
	int		len;

	if (argc == 1) {
		while (*env)
			printf("%s\n", *env++);
		return;
	}

	len = strlen(argv[1]);
	while (*env) {
		if ((strlen(*env) > len) && (env[0][len] == '=') &&
			(memcmp(argv[1], *env, len) == 0))
		{
			printf("%s\n", &env[0][len+1]);
			return;
		}
		env++;
	}
}
#endif /* CMD_PRINTENV */

#ifdef CMD_UMASK
void
do_umask(argc, argv)
	char	**argv;
{
	char	*cp;
	int	mask;

	if (argc <= 1) {
		mask = umask(0);
		umask(mask);
		printf("%03o\n", mask);
		return;
	}

	mask = 0;
	cp = argv[1];
	while (isoctal(*cp))
		mask = mask * 8 + *cp++ - '0';

	if (*cp || (mask & ~0777)) {
		fprintf(stderr, "Bad umask value\n");
		return;
	}

	umask(mask);
}
#endif /* CMD_UMASK */

#ifdef CMD_KILL
void
do_kill(argc, argv)
	char	**argv;
{
	char	*cp;
	int	sig;
	int	pid;

	sig = SIGTERM;

	if (argv[1][0] == '-') {
		cp = &argv[1][1];
		if (strcmp(cp, "HUP") == 0)
			sig = SIGHUP;
		else if (strcmp(cp, "INT") == 0)
			sig = SIGINT;
		else if (strcmp(cp, "QUIT") == 0)
			sig = SIGQUIT;
		else if (strcmp(cp, "KILL") == 0)
			sig = SIGKILL;
		else {
			sig = 0;
			while (isdecimal(*cp))
				sig = sig * 10 + *cp++ - '0';

			if (*cp) {
				fprintf(stderr, "Unknown signal\n");
				return;
			}
		}
		argc--;
		argv++;
	}

	while (argc-- > 1) {
		cp = *++argv;
		pid = 0;
		while (isdecimal(*cp))
			pid = pid * 10 + *cp++ - '0';

		if (*cp) {
			fprintf(stderr, "Non-numeric pid\n");
			return;
		}

		if (kill(pid, sig) < 0)
			perror(*argv);
	}
}
#endif /* CMD_KILL */

/* END CODE */
