/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * The "ls" built-in command.
 *
 * 17-Jan-1997 stevew@home.com
 *		- Added -C option to print 5 across screen.
 * 30-Jan-1998 ajr@ecs.soton.ac.uk (Al Riddoch)
 *		- Made -C default behavoir.
 * 02-Feb-1998 claudio@conectiva.com (Claudio Matsuoka)
 *		- Options -a, -F and simple multicolumn output added
 * 28-Nov-1999 mario.frasca@home.ict.nl (Mario Frasca)
 *		- Options -R -r added
 *		- Modified parsing of switches
 *		- This is a main rewrite, resulting in more compact code.
 * 13-Jan-2002 rhw@MemAlpha.cx (Riley Williams)
 *		- Reformatted source consistently.
 *		- Added -A option: -a excluding . and ..
 */

#if !defined(DEBUG)
#    define DEBUG 0
#endif

#if DEBUG & 1
#    define TRACESTRING(a) printf("%s\n", a);
#else
#    define TRACESTRING(a)
#endif

#include "futils.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <string.h>
#include <grp.h>
#include <time.h>


#define LISTSIZE 256

/* klugde */
#define COLS 80


#ifdef S_ISLNK
#    define LSTAT lstat
#else
#    define LSTAT stat
#endif


/*
 * Flags for the LS command.
 */
#define LSF_LONG	0x01
#define LSF_DIR 	0x02
#define LSF_INODE	0x04
#define LSF_MULT	0x08
#define LSF_ALL 	0x10	/* List files starting with `.' */
#define LSF_ALLX	0x20	/* List . files except . and .. */
#define LSF_CLASS	0x40	/* Classify files (append symbol) */

#define isntDotDir(name) *name!='.' || \
			( (c=name[1]) && (c!='.') ) || (name[2]&&c)


static int cols = 0, col = 0, reverse=-1;
static char fmt[16] = "%s";


static void lsfile();
static void setfmt();

int namesort(a, b)
const char **a, **b;
{
    return reverse*strcmp(*a, *b);
}

struct Stack
{
    int size, allocd;
    char **buf;
};

void initStack( pstack )
struct Stack *pstack;
{
    pstack->size = 0;
    pstack->allocd = 0;
    pstack->buf = NULL;
}

char * popStack( pstack )
struct Stack * pstack;
{
    return (pstack->size)?pstack->buf[--(pstack->size)]:NULL;
}

void pushStack( pstack, entry )
struct Stack * pstack;
char * entry;
{
    if ( pstack->size == pstack->allocd ) {
	(pstack->allocd) += 8;
	pstack->buf = (char**)realloc( pstack->buf, sizeof(char*)*pstack->allocd );
  }
  pstack->buf[(pstack->size)++] = entry;
}

void printStack(pstack)
struct Stack *pstack;
{
    int i;

    for (i=0; i<pstack->size; i++)
	printf("%d : %s\n", i, pstack->buf[i]);
}

void sortStack( pstack )
struct Stack *pstack;
{
    qsort( pstack->buf, pstack->size, sizeof(char*), namesort );
}

int isEmptyStack( pstack )
struct Stack *pstack;
{
    return !(pstack->size);
}

void getfiles(name, pstack, flags)
char * name;
struct Stack *pstack;
int flags;
{
    BOOL endslash, valid;
    DIR *dirp;
    struct dirent *dp;
    char fullname[PATHLEN];
    char c;

    endslash = name[strlen(name)-1] == '/';

    /*
     * Do all the files in a directory.
     */

    dirp = opendir(name);
    if (dirp == NULL) {
	perror(name);
	exit(1);
    }
    while ((dp = readdir(dirp)) != NULL) {
	valid = 0;
	if ((flags & LSF_ALL) || (*dp->d_name != '.'))
	    valid = 1;
	else if ((flags & LSF_ALLX) && (dp->d_name[1])
			&& (dp->d_name[1] != '.' || dp->d_name[2]))
	    valid = 1;
	if (valid) {
	    *fullname = '\0';
	    strcpy(fullname, name);
	    if (!endslash)
		strcat(fullname, "/");
	    strcat(fullname, dp->d_name);
	    pushStack(pstack,strdup(fullname));
	}
    }
    closedir(dirp);
    sortStack(pstack);
}


/*
 * Do an LS of a particular file name according to the flags.
 */
static void
lsfile(name, statbuf, flags)
char *name;
struct stat *statbuf;
{
    char		*cp;
    struct passwd	*pwd;
    struct group	*grp;
    long		len;
    char		buf[PATHLEN];
    static char		username[12];
    static int		userid;
    static BOOL		useridknown;
    static char		groupname[12];
    static int		groupid;
    static BOOL		groupidknown;
    char		*class;

    cp = buf;
    *cp = '\0';

    if (flags & LSF_INODE) {
	sprintf(cp, "%5ld ", statbuf->st_ino);
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
#ifdef S_ISSOCK
	else if (S_ISSOCK (statbuf->st_mode))
	    *class = '=';
#endif
    }
    {
	char *cp;

	cp = strrchr(name, '/');
	if (!cp)
	    cp = name;
	else
	    cp++;
	printf(fmt, cp);
    }

#ifdef S_ISLNK
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

/*
 * Return the standard ls-like mode string from a file mode.
 * This is static and so is overwritten on each call.
 */
char *
modestring(mode)
{
    static char buf[12];

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
#ifdef S_ISLNK
    if (S_ISLNK(mode))
	buf[0] = 'l';
#endif
#ifdef S_ISSOCK
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
long t;
{
    long  now;
    char  *str;
    static char buf[26];

    time(&now);

    str = ctime(&t);

    strcpy(buf, &str[4]);
    buf[12] = '\0';

    if ((t > now) || (t < now - 180*24*60*60L)) {
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
char *dirname;
char *filename;
{
    char  *cp;
    static char buf[PATHLEN];

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


void setfmt(pstack, flags)
struct Stack *pstack;
{
    int maxlen, i, len;
    char * cp;

    if (~flags & LSF_LONG) {
	for (maxlen = i = 0; i < pstack->size; i++) {
	    if ( NULL != (cp = strrchr(pstack->buf[i], '/')) )
		cp++;
	    else
		cp = pstack->buf[i];
	    if ((len = strlen (cp)) > maxlen)
		maxlen = len;
	}
	maxlen += 2;
	cols = (COLS - 1) / maxlen;
	sprintf (fmt, "%%-%d.%ds", maxlen, maxlen);
    }
}


int main(argc, argv)
int argc;
char **argv;
{
    char  *cp;
    char  *name;
    int  flags, recursive, isDir;
    struct stat statbuf;
    static char *def[] = {".", 0};
    struct Stack files, dirs;

    initStack(&files);
    initStack(&dirs);

    flags = 0;
    recursive = 1;

/*
 * Set relevant flags for command name
 */
    if (!strcmp(*argv,"dir"))
	flags |= LSF_LONG;

    while ( --argc && ((cp = * ++argv)[0]=='-') ) {
	while (*++cp) {
	    switch(*cp) {
		case 'l':
			flags |= LSF_LONG;
			break;
		case 'd':
			flags |= LSF_DIR;
			recursive = 0;
			break;
		case 'R':
			recursive = -1;
			break;
		case 'i':
			flags |= LSF_INODE;
			break;
		case 'a':
			flags |= LSF_ALL;
			break;
		case 'A':
			flags |= LSF_ALLX;
			break;
		case 'F':
			flags |= LSF_CLASS;
			break;
		case 'r':
			reverse = -reverse;
			break;
		default:
			if (~flags)
			    fputs("Unknown option: ", stderr);
			fputc(*cp, stderr);
			flags = -1;
			break;
	    }
	}
    }
    if (flags == -1) {
	fputc('\n', stderr);
	exit(1);
    }
    if (!argc) {
	argv = def;
	argc = 1;
    }
    TRACESTRING(*argv)
    if (argv[1])
	flags |= LSF_MULT;

    for ( ; *argv; argv++) {
	if (LSTAT(*argv, &statbuf) < 0) {
	    perror(*argv);
	    exit(1);
	}
	if (recursive && S_ISDIR(statbuf.st_mode))
	    pushStack(&dirs, strdup(*argv) );
	else
	    pushStack(&files, strdup(*argv) );
    }
    if (recursive)
	recursive--;
    sortStack(&files);
    do {
	setfmt(&files, flags);
/*	if (flags & LSF_MULT)
	    printf("\n%s:\n", name);
 */
	while (!isEmptyStack(&files)) {
	    name = popStack(&files);
	    TRACESTRING(name)
	    if (LSTAT(name, &statbuf) < 0) {
		perror(name);
		free(name);
		continue;
	    }
	    isDir = S_ISDIR(statbuf.st_mode);
	    if (!isDir || !recursive || (flags&LSF_LONG))
		lsfile(name, &statbuf, flags);
	    if (isDir && recursive)
		pushStack( &dirs, name);
	    else
		free(name);
	}
	if (!isEmptyStack(&dirs)) {
	    getfiles( name = popStack(&dirs), &files, flags );
	    if (strcmp(name,".")) {
		if (col) {
		    col=0;
		    fputc('\n', stdout);
		}
		printf("\n%s:\n", name);
	    }
	    free(name);
	    if (recursive)
		recursive--;
	}
    } while (!isEmptyStack(&files) || !isEmptyStack(&dirs));
    if (~flags & LSF_LONG)
	fputc('\n', stdout);
    return 0;
}
