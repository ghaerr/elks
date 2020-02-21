/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * The "ls" built-in command.
 */

#include "sash.h"
#ifdef CMD_LS

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>


#define	LISTSIZE	256


#ifdef	S_ISLNK
#define	LSTAT	lstat
#else
#define	LSTAT	stat
#endif


/*
 * Flags for the LS command.
 */
#define	LSF_LONG	0x01
#define	LSF_DIR		0x02
#define	LSF_INODE	0x04
#define	LSF_MULT	0x08


static	char	**list;
static	int	listsize;
static	int	listused;


static	void	lsfile();


void
do_ls(argc, argv)
	char	**argv;
{
	char		*cp;
	char		*name;
	int		flags;
	int		i;
	DIR		*dirp;
	BOOL		endslash;
	char		**newlist;
	struct	dirent	*dp;
	char		fullname[PATHLEN];
	struct	stat	statbuf;
	static		char *def[2] = {"-ls", "."};

	if (listsize == 0) {
		list = (char **) malloc(LISTSIZE * sizeof(char *));
		if (list == NULL) {
			fprintf(stderr, "No memory for ls buffer\n");
			return;
		}
		listsize = LISTSIZE;
	}
	listused = 0;

	flags = 0;
	if ((argc > 1) && (argv[1][0] == '-'))
	{
		argc--;
		cp = *(++argv) + 1;

		while (*cp) switch (*cp++) {
			case 'l':	flags |= LSF_LONG; break;
			case 'd':	flags |= LSF_DIR; break;
			case 'i':	flags |= LSF_INODE; break;
			default:
				fprintf(stderr, "Unknown option -%c\n", cp[-1]);
				return;
		}
	}

	if (argc <= 1) {
		argc = 2;
		argv = def;
	}

	if (argc > 2)
		flags |= LSF_MULT;

	while (argc-- > 1) {
		name = *(++argv);
		endslash = (*name && (name[strlen(name) - 1] == '/'));

		if (LSTAT(name, &statbuf) < 0) {
			perror(name);
			continue;
		}

		if ((flags & LSF_DIR) || (!S_ISDIR(statbuf.st_mode))) {
			lsfile(name, &statbuf, flags);
			continue;
		}

		/*
		 * Do all the files in a directory.
		 */
		dirp = opendir(name);
		if (dirp == NULL) {
			perror(name);
			continue;
		}

		if (flags & LSF_MULT)
			printf("\n%s:\n", name);

		while ((dp = readdir(dirp)) != NULL) {
			if (intflag)
				break;

			fullname[0] = '\0';

			if ((*name != '.') || (name[1] != '\0')) {
				strcpy(fullname, name);
				if (!endslash)
					strcat(fullname, "/");
			}

			strcat(fullname, dp->d_name);

			if (listused >= listsize) {
				newlist = realloc(list,
					((sizeof(char **)) * (listsize + LISTSIZE)));
				if (newlist == NULL) {
					fprintf(stderr, "No memory for ls buffer\n");
					break;
				}
				list = newlist;
				listsize += LISTSIZE;
			}

			list[listused] = strdup(fullname);
			if (list[listused] == NULL) {
				fprintf(stderr, "No memory for filenames\n");
				break;
			}
			listused++;
		}

		closedir(dirp);

		/*
		 * Sort the files.
		 */
		qsort((char *) list, listused, sizeof(char *), namesort);

		/*
		 * Now finally list the filenames.
		 */
		for (i = 0; i < listused; i++) {
			name = list[i];

			if (LSTAT(name, &statbuf) < 0) {
				perror(name);
				free(name);
				continue;
			}

			cp = strrchr(name, '/');
			if (cp)
				cp++;
			else
				cp = name;

			lsfile(cp, &statbuf, flags);

			free(name);
		}

		listused = 0;
	}
	fflush(stdout);
}


/*
 * Do an LS of a particular file name according to the flags.
 */
static void
lsfile(name, statbuf, flags)
	char	*name;
	struct	stat	*statbuf;
{
	char		*cp;
	struct	passwd	*pwd;
	struct	group	*grp;
	long		len;
	char		buf[PATHLEN];
	static	char	username[12];
	static	int	userid;
	static	BOOL	useridknown;
	static	char	groupname[12];
	static	int	groupid;
	static	BOOL	groupidknown;

	cp = buf;
	*cp = '\0';

	if (flags & LSF_INODE) {

#ifdef CONFIG_32BIT_INODES
		sprintf(cp, "%5ld ", statbuf->st_ino);
#else
		sprintf(cp, "%5d ", statbuf->st_ino);
#endif

		cp += strlen(cp);
	}

	if (flags & LSF_LONG) {
		strcpy(cp, modestring(statbuf->st_mode));
		cp += strlen(cp);

		sprintf(cp, "%3d ", statbuf->st_nlink);
		cp += strlen(cp);

		if (!useridknown || (statbuf->st_uid != userid)) {
			pwd = getpwuid(statbuf->st_uid);
			if (pwd)
				strcpy(username, pwd->pw_name);
			else
				sprintf(username, "%d", statbuf->st_uid);
			userid = statbuf->st_uid;
			useridknown = TRUE;
		}

		sprintf(cp, "%-8s ", username);
		cp += strlen(cp);

		if (!groupidknown || (statbuf->st_gid != groupid)) {
			grp = getgrgid(statbuf->st_gid);
			if (grp)
				strcpy(groupname, grp->gr_name);
			else
				sprintf(groupname, "%d", statbuf->st_gid);
			groupid = statbuf->st_gid;
			groupidknown = TRUE;
		}

		sprintf(cp, "%-8s ", groupname);
		cp += strlen(cp);

		if (S_ISBLK(statbuf->st_mode) || S_ISCHR(statbuf->st_mode))
			sprintf(cp, "%3d, %3d ", statbuf->st_rdev >> 8,
				statbuf->st_rdev & 0xff);
		else
			sprintf(cp, "%8ld ", statbuf->st_size);
		cp += strlen(cp);

		sprintf(cp, " %-12s ", timestring(statbuf->st_mtime));
	}

	fputs(buf, stdout);
	fputs(name, stdout);

#ifdef	S_ISLNK
	if ((flags & LSF_LONG) && S_ISLNK(statbuf->st_mode)) {
		len = readlink(name, buf, PATHLEN - 1);
		if (len >= 0) {
			buf[len] = '\0';
			printf(" -> %s", buf);
		}
	}
#endif

	fputc('\n', stdout);
}

#endif /* CMD_LS */

/* END CODE */
