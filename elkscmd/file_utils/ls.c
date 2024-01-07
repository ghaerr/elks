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
 * 28-May-2004 claudio@conectiva.com (Claudio Matsuoka)
 *		- Fixed sort direction, keeps qsort and strcmp consistent
 *		- Removed alias to 'dir' (doesn't seem to belong here)
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <string.h>
#include <grp.h>
#include <time.h>
#include <limits.h>

/* klugde */
#define COLS 80

#ifdef S_ISLNK
#    define LSTAT lstat
#else
#    define LSTAT stat
#endif

/* Flags for the LS command */
#define LSF_LONG	0x01
#define LSF_DIR 	0x02
#define LSF_INODE	0x04
#define LSF_MULT	0x08
#define LSF_ALL 	0x10	/* List files starting with `.' */
#define LSF_ALLX	0x20	/* List . files except . and .. */
#define LSF_CLASS	0x40	/* Classify files (append symbol) */
#define LSF_ONEPER	0x80	/* One entry per line */

static void lsfile();
static void setfmt();
static char *modestring(int mode);
static char *timestring(time_t t);

struct sort {
    char *name;
    long longval;
};

struct stack
{
    int size, allocd;
    struct sort *buf;
};

static int cols, col;
static int reverse = -1;
static int sortbytime;
static int sortbysize;
static int nosort;
static char fmt[16] = "%s";

/* return -1/0/1 based on sign of x */
static int sign(long x)
{
    return (x > 0) - (x < 0);
}

static int namesort(const struct sort *a, const struct sort *b)
{
    if (sortbytime || sortbysize) {
        return sign(reverse * (b->longval - a->longval));
    }
    return reverse * strcmp(a->name, b->name);
}

static void initstack(struct stack *pstack)
{
    pstack->size = 0;
    pstack->allocd = 0;
    pstack->buf = NULL;
}

static char *popstack(struct stack *pstack)
{
    return (pstack->size)?pstack->buf[--(pstack->size)].name:NULL;
}

static void pushstack(struct stack *pstack, char *entry, long l)
{
    struct sort *allocbuf;

    if (pstack->size == pstack->allocd) {
        pstack->allocd += 64;
        allocbuf = (struct sort*)realloc(pstack->buf, sizeof(struct sort)*pstack->allocd);
        if (!allocbuf) {
            fprintf(stderr, "ls: error: out of memory\n");
            exit(EXIT_FAILURE);
        }
        pstack->buf = allocbuf;
    }
    pstack->buf[pstack->size].longval = l;
    pstack->buf[pstack->size++].name = entry;
}

static void sortstack(struct stack *pstack)
{
    if (nosort == 0)
        qsort(pstack->buf, pstack->size, sizeof(struct sort), namesort);
}

static int getfiles(char *name, struct stack *pstack, int flags)
{
    int addslash;
    DIR *dirp;
    struct dirent *dp;
    char fullname[PATH_MAX];
    int pathlen = strlen(name);

    addslash = name[pathlen - 1] != '/';
    if (pathlen + addslash >= sizeof(fullname)) {
toolong:
        fputs("Pathname too long\n", stderr);
        return -1;
    }
    memcpy(fullname, name, pathlen + 1);
    if (addslash) {
        strcat(fullname + pathlen, "/");
        pathlen++;
    }

    /*
     * Do all the files in a directory.
     */

    dirp = opendir(name);
    if (dirp == NULL) {
        perror(name);
        return -1;
    }
    while ((dp = readdir(dirp)) != NULL) {
        if ((flags & LSF_ALL) || (*dp->d_name != '.') ||
            ((flags & LSF_ALLX) && (dp->d_name[1])
                && (dp->d_name[1] != '.' || dp->d_name[2]))) {
            int namelen = strlen(dp->d_name);
            if (pathlen + namelen >= sizeof(fullname)) {
                closedir(dirp);
                goto toolong;
            }
            memcpy(fullname + pathlen, dp->d_name, namelen + 1);
            long l = 0;
            if (sortbytime || sortbysize) {
                struct stat statbuf;
                if (LSTAT(fullname, &statbuf) >= 0)
                    l = sortbytime? statbuf.st_mtime: statbuf.st_size;
            }
            pushstack(pstack, strdup(fullname), l);
        }
    }
    closedir(dirp);
    sortstack(pstack);
    return 0;
}


/*
 * Do an LS of a particular file name according to the flags.
 */
static void lsfile(char *name, struct stat *statbuf, int flags)
{
    char		*cp;
    struct passwd	*pwd;
    struct group	*grp;
    long		len;
    static int		userid;
    static int		useridknown;
    static int		groupid;
    static int		groupidknown;
    char		class;
    char		*classp;
    char		*pp;
    static char		username[12];
    static char		groupname[12];
    char		buf[PATH_MAX];
    struct stat		sbuf;

    cp = buf;
    *cp = '\0';

    if (flags & (LSF_INODE|LSF_LONG|LSF_CLASS) && !statbuf) {
        if (LSTAT(name, &sbuf) < 0) {
            perror(name);
            return;
        }
        statbuf = &sbuf;
    }

    if (flags & LSF_INODE) {
        cp += sprintf(cp, "%5lu ", (unsigned long)statbuf->st_ino);
    }

    if (flags & LSF_LONG) {
        strcpy(cp, modestring(statbuf->st_mode));
        cp += strlen(cp);

        cp += sprintf(cp, "%3lu ", (unsigned long)statbuf->st_nlink);

        if (!useridknown || (statbuf->st_uid != userid)) {
            pwd = getpwuid(statbuf->st_uid);
            if (pwd)
                strcpy(username, pwd->pw_name);
            else
                sprintf(username, "%d", statbuf->st_uid);
            userid = statbuf->st_uid;
            useridknown = 1;
        }

        cp += sprintf(cp, "%-8s ", username);

        if (!groupidknown || (statbuf->st_gid != groupid)) {
            grp = getgrgid(statbuf->st_gid);
            if (grp)
                strcpy(groupname, grp->gr_name);
            else
                sprintf(groupname, "%d", statbuf->st_gid);
            groupid = statbuf->st_gid;
            groupidknown = 1;
        }

        cp += sprintf(cp, "%-8s ", groupname);

        if (S_ISBLK(statbuf->st_mode) || S_ISCHR(statbuf->st_mode))
            cp += sprintf(cp, "%3lu, %3lu ", (unsigned long)(statbuf->st_rdev >> 8),
                    (unsigned long)(statbuf->st_rdev & 0xff));
        else
            cp += sprintf(cp, "%8lu ", (unsigned long)statbuf->st_size);

        sprintf(cp, " %-12s ", timestring(statbuf->st_mtime));
    }

    fputs(buf, stdout);

    class = '\0';
    if (flags & LSF_CLASS) {
        if (S_ISLNK(statbuf->st_mode))
            class = '@';
        else if (S_ISDIR(statbuf->st_mode))
            class = '/';
        else if (S_IEXEC & statbuf->st_mode)
            class = '*';
        else if (S_ISFIFO(statbuf->st_mode))
            class = '|';
#ifdef S_ISSOCK
        else if (S_ISSOCK(statbuf->st_mode))
            class = '=';
#endif
    }

    int buflen = strlen(name);
    memcpy(buf, name, buflen + 1);
    pp = strrchr(buf, '/');

    /* If a class character exists for the file name, add it on */
    if (class != '\0') {
        classp = &buf[buflen];
        *classp++ = class;
        *classp = '\0';
    }

    if (!pp) pp = buf;
    else pp++;
    if (flags & LSF_ONEPER)
        printf("%s", pp);	/* One per line: No trailing spaces! */
    else
        printf(fmt, pp);

#ifdef S_ISLNK
    if ((flags & LSF_LONG) && S_ISLNK(statbuf->st_mode)) {
        len = readlink(name, buf, PATH_MAX - 1);
        if (len >= 0) {
            buf[len] = '\0';
            printf(" -> %s", buf);
        }
    }
#endif

    if ((flags & (LSF_LONG|LSF_ONEPER)) || ++col == cols) {
        fputc('\n', stdout);
        col = 0;
    }
}

/*
 * Return the standard ls-like mode string from a file mode.
 * This is static and so is overwritten on each call.
 */
static char *modestring(int mode)
{
    static char buf[12];

    strcpy(buf, "----------");

    /*
     * Fill in the file type.
     */

    if (S_ISDIR(mode))
        buf[0] = 'd';
    else if (S_ISCHR(mode))
        buf[0] = 'c';
    else if (S_ISBLK(mode))
        buf[0] = 'b';
    else if (S_ISFIFO(mode))
        buf[0] = 'p';
#ifdef S_ISLNK
    else if (S_ISLNK(mode))
        buf[0] = 'l';
#endif
#ifdef S_ISSOCK
    else if (S_ISSOCK(mode))
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
static char *timestring(time_t t)
{
    time_t  now;
    char  *str;
    static char buf[26];

    time(&now);

    str = ctime(&t);

    strcpy(buf, &str[4]);
    buf[12] = '\0';

    if ((t > now) || (t < now - 180*24*60L*60)) {
        buf[7] = ' ';
        strcpy(&buf[8], &str[20]);
        buf[12] = '\0';
    }

    return buf;
}


static void setfmt(struct stack *pstack, int flags)
{
    int maxlen, maxlen2, i, len;
    char * cp;

    if (~flags & LSF_LONG) {
        for (maxlen = i = 0; i < pstack->size; i++) {
            if ( NULL != (cp = strrchr(pstack->buf[i].name, '/')) )
                cp++;
            else
                cp = pstack->buf[i].name;
            if ((len = strlen (cp)) > maxlen)
                maxlen = len;
        }
        maxlen += 2;
        maxlen2 = flags & LSF_INODE? maxlen + 6: maxlen;
        cols = (COLS - 1) / maxlen2;
        sprintf (fmt, "%%-%d.%ds", maxlen, maxlen);
    }
}


int not_dotdir(char *name)
{
    char *p = strrchr(name, '/');
    return !(p && p[1] == '.' && (!p[2] || (p[2] == '.' && !p[3])));
}


int main(int argc, char **argv)
{
    char  *cp;
    char  *name = argv[0];
    int  status = EXIT_SUCCESS;
    int  flags, recursive, is_dir;
    struct stat statbuf;
    static char *def[] = {".", 0};
    struct stack files, dirs;

    initstack(&files);
    initstack(&dirs);

    flags = 0;
    recursive = 1;

    /*
     * Set relevant flags for command name
     */

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
                case '1':
                    flags |= LSF_ONEPER;
                    break;
                case 't':
                    sortbytime = 1;
                    break;
                case 'S':
                    sortbysize = 1;
                    break;
                case 'r':
                    reverse = -reverse;
                    break;
                case 'U':
                    nosort = 1;
                    break;
                default:
                    if (~flags) fprintf(stderr, "unknown option '%c'\n", *cp);
                    goto usage;
            }
        }
    }
    if (!argc) {
        argv = def;
        argc = 1;
    }
    TRACESTRING(*argv)
    if (argv[1])
        flags |= LSF_MULT;
    if (!isatty(1))
        flags |= LSF_ONEPER;

    for ( ; *argv; argv++) {
        if (LSTAT(*argv, &statbuf) < 0) {
            perror(*argv);
            return EXIT_FAILURE;
        }
        if (recursive && S_ISDIR(statbuf.st_mode))
            pushstack(&dirs, strdup(*argv), statbuf.st_mtime);
        else
            pushstack(&files, strdup(*argv), statbuf.st_mtime);
    }
    if (recursive)
        recursive--;
    sortstack(&files);
    do {
        setfmt(&files, flags);
        /* if (flags & LSF_MULT)
               printf("\n%s:\n", name);
         */
        while (files.size) {
            int didls = 0;
            name = popstack(&files);
            TRACESTRING(name)
            if (!recursive || (flags & LSF_LONG)) {
                lsfile(name, NULL, flags);
                didls = 1;
                if (!recursive) {
                    free(name);
                    continue;
                }
            }
            if (LSTAT(name, &statbuf) < 0) {
                perror(name);
                free(name);
                continue;
            }
            is_dir = S_ISDIR(statbuf.st_mode);
            if (!didls)
                lsfile(name, &statbuf, flags);
            if (is_dir && recursive && not_dotdir(name))
                pushstack(&dirs, name, statbuf.st_mtime);
            else
                free(name);
        }
        if (dirs.size) {
            if (getfiles(name = popstack(&dirs), &files, flags) != 0) {
                status = EXIT_FAILURE;
            } else if (strcmp(name, ".")) {
                if (col) {
                    col = 0;
                    fputc('\n', stdout);
                }
                if ((flags & LSF_MULT) || recursive)
                    printf("\n%s:\n", name);
            }
            free(name);
            if (recursive) recursive--;
        }
    } while (files.size || dirs.size);
    if (!(flags & (LSF_LONG|LSF_ONEPER)) && col)
        fputc('\n', stdout);
    return status;

usage:
    fprintf(stderr, "usage: %s [-aAFiltSrR1U] [name ...]\n", name);
    return EXIT_FAILURE;
}
