/*
 * misc functions
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>
#include <paths.h>
#include "cron.h"

#ifdef ELKS
#define	LOG_ERR		3       /* error conditions */
#define	LOG_INFO	6       /* informational */
#endif

int interactive = 0;
int lineno;
time_t t_now;

extern char *pgm;


/*
 * print the (hex) value of an event mask
 */
#if DEBUG
void
printtrig(Evmask *m)
{
    printf("%08lx-%08lx ", m->minutes[0], m->minutes[1]);
    printf("%08lx ", m->hours);
    printf("%08lx ", m->mday);
    printf("%04x ", m->month);
    printf("%04x ", m->wday);
}
#else
void
printtrig(Evmask *m)
{
    (void)m;
}
#endif


/*
 * (re)allocate memory or die.
 */
void *
xrealloc(void *elp, int nrel, int szel)
{
    void *ret;

    if (ret = elp ? realloc(elp, nrel * szel) : malloc(nrel * szel))
        return ret;

    fatal("xrealloc: %s", strerror(errno));
    return 0;
}


/*
 * complain about something, sending the message either to stderr or syslog,
 * depending on whether (interactive) is set.
 */
static void
_error(int severity, char *fmt, va_list ptr)
{
    FILE *ftmp;

    if (interactive) {
        vfprintf(stderr, fmt, ptr);
        fputc('\n', stderr);
    } else {
        if ((ftmp = fopen(_PATH_CRONLOG, "a+")) == 0) {
            return;
        }
        vfprintf(ftmp, fmt, ptr);
        time(&t_now);
        fprintf(ftmp, "  -  %s", ctime(&t_now));
        fclose(ftmp);
    }
}


/*
 * complain about something
 */
void
error(char *fmt,...)
{
    va_list ptr;

    va_start(ptr, fmt);
    _error(LOG_ERR, fmt, ptr);
    va_end(ptr);
}


/*
 * complain about something, then die.
 */
void
fatal(char *fmt,...)
{
    va_list ptr;

    va_start(ptr, fmt);
    _error(LOG_ERR, fmt, ptr);
    va_end(ptr);
    exit(1);
}


/*
 * log something non-fatal
 */
void
info(char *fmt,...)
{
    va_list ptr;

    va_start(ptr, fmt);
    _error(LOG_INFO, fmt, ptr);
    va_end(ptr);
}


/*
 * get a logical line of unlimited size.  Convert unescaped %'s into
 * newlines, and concatenate lines together if escaped with \ at eol.
 */
char *
fgetlol(FILE *f)
{
    static char *line = 0;
    static int szl = 0;
    int nrl = 0;
    int c;

    while ((c = fgetc(f)) != EOF) {
        EXPAND(line, szl, nrl + 1);

        if (c == '\\') {
            if ((c = fgetc(f)) == EOF)
                break;
            if (c == '\n')
                ++lineno;
            else if (c != '%')
                line[nrl++] = '\\';
            line[nrl++] = c;
            continue;
        } else if (c == '%')
            c = '\n';
        else if (c == '\n') {
            ++lineno;
            line[nrl] = 0;
            return line;
        }

        line[nrl++] = c;
    }
    if (nrl) {
        line[nrl] = 0;
        return line;
    }
    return 0;
}


/*
 * return the mtime of a file (or the current time of day if the stat()
 * failed to work
 */
time_t
mtime(char *path){
    struct stat st;
    time_t now;

    if (stat(path, &st) == 0)
        return st.st_mtime;

    error("can't stat %s -- returning current time", path);
    time(&now);
    return now;
}


/*
 * erase the contents of a crontab, leaving the arrays allocated for later
 * use.
 */
void
zerocrontab(crontab *tab)
{
    int i;

    if (tab == 0)
        return;

    for (i = 0; i < tab->nre; ++i)
        if (tab->env[i])
            free(tab->env[i]);
    tab->nre = 0;

    for (i = 0; i < tab->nrl; ++i) {
        free(tab->list[i].command);
        if (tab->list[i].input)
            free(tab->list[i].input);
    }
    tab->nrl = 0;
}


/*
 * eat leading blanks on a line
 */
char *
firstnonblank(char *s)
{
    while (*s && isspace(*s))
        ++s;

    return s;
}


/*
 * change directory or die
 */
void
xchdir(char *path)
{
    if (chdir(path) == -1)
        fatal("%s: %s", path, strerror(errno));
}

int
xis_crondir(void)
{
    FILE *ftmp;

    DIR *dir = opendir(_PATH_CRONDIR);
    if (dir) {                  /* Directory exists. */
        closedir(dir);
    } else {                    /* Directory does not exist or opendir
                                 * failed. */
        int result = mkdir(_PATH_CRONDIR, 0777);        /*-1 fail, 0 ok */
        if (result == -1)
            return -1;
        /* created CRONDIR, generate root crontab file template */
        if ((ftmp = fopen(_PATH_CRONTAB, "a+")) == 0) {
            error("can't create crontab template: %s", strerror(errno));
            return -1;
        }
        fprintf(ftmp, "* * * * * echo Hello! Here is cron >>cron.out\n");
        fclose(ftmp);
    }
    return 0;
}

#ifdef ELKS                     /* setregid not defined */

int
setregid(gid_t rgid, gid_t egid)
{
#ifdef HAVE_SETEGID
    if (setegid(egid) != 0)
        return -1;
#endif
    if (setgid(rgid) != 0)
        return -1;
    return 0;
}

#endif


#ifdef ELKS                     /* setreuid not defined */

int
setreuid(gid_t ruid, gid_t euid)
{
#ifdef HAVE_SETEUID
    if (seteuid(euid) != 0)
        return -1;
#endif
    if (setuid(ruid) != 0)
        return -1;
    return 0;
}

#endif

#define	ptr(base, size, i)	((const void *)(((unsigned char *)(base)) + ((i) * (size))))

void *
bsearch_c(const void *key, const void *base, size_t nmemb, size_t size,
          int (*compar) (const void *, const void *))
{
    if (nmemb) {
        size_t i = nmemb >> 1;
        void const *p = ptr(base, size, i);
        int k = compar(key, p);

        if (k) {
            if (k < 0)
                return bsearch_c(key, base, i, size, compar);
            else
                return bsearch_c(key, ((unsigned char *)p) + size, nmemb - (i + 1), size, compar);
        }

        return (void *)p;
    }

    return NULL;
}
