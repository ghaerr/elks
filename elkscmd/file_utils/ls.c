/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * The "ls" built-in command.
 *
 * 1/17/97  stevew@home.com   Added -C option to print 5 across
 *                             screen.
 *
 * 30-Jan-98 ajr@ecs.soton.ac.uk  Made -C default behavoir.
 *
 * Mon Feb  2 19:04:14 EDT 1998 - claudio@pos.inf.ufpr.br (Claudio Matsuoka)
 * Options -a, -F and simple multicolumn output added
 */

#include "futils.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>


#define	LISTSIZE	256

/* klugde */
#define COLS 80


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
#define LSF_ALL		0x10		/* List files starting with `.' */
#define LSF_CLASS	0x20		/* Classify files (append symbol) */


static	char	**list;
static	int	listsize;
static	int	listused;
static	int	cols = 0, col = 0;
static	char	fmt[10] = "%s";


static	void	lsfile();

void
main(argc, argv)
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
	int		len, maxlen;
	int		num;

	if (listsize == 0) {
		list = (char **) malloc(LISTSIZE * sizeof(char *));
		if (list == NULL) {
			fprintf(stderr, "No memory for ls buffer\n");
			exit(1);
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
			case 'a':	flags |= LSF_ALL; break;
			case 'F':	flags |= LSF_CLASS; break;
			default:
				fprintf(stderr, "Unknown option -%c\n", cp[-1]);
				exit(1);
		}
	}

	if (argc <= 1) {
		argc = 2;
		argv = def;
	}

	if (argc > 2)
		flags |= LSF_MULT;

	while (argc-- > 1) {
		if ((name = malloc (strlen (*(++argv)) + 2)) == NULL) {
			fprintf(stderr, "No memory for filenames\n");
			exit(1);
		}
		strcpy (name, *argv);

		endslash = (*name && (name[strlen(name) - 1] == '/'));

		if (LSTAT(name, &statbuf) < 0) {
			perror(name);
			free(name);
			continue;
		}

		if ((flags & LSF_DIR) || (!S_ISDIR(statbuf.st_mode))) {
			lsfile(name, &statbuf, flags);
			fputc('\n', stdout);
			free(name);
			continue;
		}

		/*
		 * Do all the files in a directory.
		 */
		dirp = opendir(name);
		if (dirp == NULL) {
			perror(name);
			free(name);
			continue;
		}

		if (flags & LSF_MULT)
			printf("\n%s:\n", name);

		while ((dp = readdir(dirp)) != NULL) {
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

			strcat(fullname, " ");
			list[listused] = strdup(fullname);

			if (list[listused] == NULL) {
				fprintf(stderr, "No memory for filenames\n");
				break;
			}

			list[listused][strlen (fullname) - 1] = 0;
			listused++;
		}

		free(name);
		closedir(dirp);

		/*
		 * Sort the files.
		 */
		qsort((char *) list, listused, sizeof(char *), namesort);

		/*
		 * Get list entry size for multi-column output.
		 */
		if (~flags & LSF_LONG) {
			for (maxlen = i = 0; i < listused; i++) {
				if (cp = strrchr(list[i], '/'))
					cp++;
				else
					cp = list[i];

				if ((len = strlen (cp)) > maxlen)
					maxlen = len;
			}
			maxlen += 2;
			cols = (COLS - 1) / maxlen;
			sprintf (fmt, "%%-%d.%ds", maxlen, maxlen);
		}
		
		/*
		 * Now finally list the filenames.
		 */
		for (num = i = 0; i < listused; i++) {
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

			if (flags & LSF_ALL || *cp != '.') {
				lsfile(cp, &statbuf, flags);
				num++;
			}

			free(name);
		}

		if ((~flags & LSF_LONG) && (num % cols))
			fputc('\n', stdout);

		listused = 0;
	}
/*	fflush(stdout); */
	exit(0);
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
	char		*class;

	cp = buf;
	*cp = '\0';

	if (flags & LSF_INODE) {
		sprintf(cp, "%5d ", statbuf->st_ino);
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

	class = name + strlen(name);
	*class = 0;
	if (flags & LSF_CLASS) {
		if (S_ISLNK (statbuf->st_mode))
			*class = '@';
		else if (S_ISDIR (statbuf->st_mode))
			*class = '/';
		else if (S_IEXEC & statbuf->st_mode)
			*class = '*';
		else if (S_ISFIFO (statbuf->st_mode))
			*class = '|';
		else if (S_ISSOCK (statbuf->st_mode))
			*class = '=';
	}
	printf(fmt, name);

#ifdef	S_ISLNK
	if ((flags & LSF_LONG) && S_ISLNK(statbuf->st_mode)) {
		len = readlink(name, buf, PATHLEN - 1);
		if (len >= 0) {
			buf[len] = '\0';
			printf(" -> %s", buf);
		}
	}
#endif

	if (flags & LSF_LONG || ++col == cols) {
		fputc('\n', stdout);
		col = 0;
	}
}

#define BUF_SIZE 1024 

typedef	struct	chunk	CHUNK;
#define	CHUNKINITSIZE	4
struct	chunk	{
	CHUNK	*next;
	char	data[CHUNKINITSIZE];	/* actually of varying length */
};


static	CHUNK *	chunklist;


/*
 * Return the standard ls-like mode string from a file mode.
 * This is static and so is overwritten on each call.
 */
char *
modestring(mode)
{
	static	char	buf[12];

	strcpy(buf, "----------");

	/*
	 * Fill in the file type.
	 */
	if (S_ISDIR(mode))
		buf[0] = 'd';
	if (S_ISCHR(mode))
		buf[0] = 'c';
	if (S_ISBLK(mode))
		buf[0] = 'b';
	if (S_ISFIFO(mode))
		buf[0] = 'p';
#ifdef	S_ISLNK
	if (S_ISLNK(mode))
		buf[0] = 'l';
#endif
#ifdef	S_ISSOCK
	if (S_ISSOCK(mode))
		buf[0] = 's';
#endif

	/*
	 * Now fill in the normal file permissions.
	 */
	if (mode & S_IRUSR)
		buf[1] = 'r';
	if (mode & S_IWUSR)
		buf[2] = 'w';
	if (mode & S_IXUSR)
		buf[3] = 'x';
	if (mode & S_IRGRP)
		buf[4] = 'r';
	if (mode & S_IWGRP)
		buf[5] = 'w';
	if (mode & S_IXGRP)
		buf[6] = 'x';
	if (mode & S_IROTH)
		buf[7] = 'r';
	if (mode & S_IWOTH)
		buf[8] = 'w';
	if (mode & S_IXOTH)
		buf[9] = 'x';

	/*
	 * Finally fill in magic stuff like suid and sticky text.
	 */
	if (mode & S_ISUID)
		buf[3] = ((mode & S_IXUSR) ? 's' : 'S');
	if (mode & S_ISGID)
		buf[6] = ((mode & S_IXGRP) ? 's' : 'S');
	if (mode & S_ISVTX)
		buf[9] = ((mode & S_IXOTH) ? 't' : 'T');

	return buf;
}

/*
 * Get the time to be used for a file.
 * This is down to the minute for new files, but only the date for old files.
 * The string is returned from a static buffer, and so is overwritten for
 * each call.
 */
char *
timestring(t)
	long	t;
{
	long		now;
	char		*str;
	static	char	buf[26];

	time(&now);

	str = ctime(&t);

	strcpy(buf, &str[4]);
	buf[12] = '\0';

	if ((t > now) || (t < now - 365*24*60*60L)) {
		strcpy(&buf[7], &str[20]);
		buf[11] = '\0';
	}

	return buf;
}

/*
 * Build a path name from the specified directory name and file name.
 * If the directory name is NULL, then the original filename is returned.
 * The built path is in a static area, and is overwritten for each call.
 */
char *
buildname(dirname, filename)
	char	*dirname;
	char	*filename;
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
 * Sort routine for list of filenames.
 */
int
namesort(p1, p2)
	char	**p1;
	char	**p2;
{
	return strcmp(*p1, *p2);
}
