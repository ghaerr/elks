/*
 * compile crontabs
 */
#include <stdio.h>
#include <pwd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <dirent.h>
#include <errno.h>

#include "cron.h"

typedef void (*ef) (Evmask *, int);

/*
 * a constraint defines part of a time spec. min & max are the smallest and
 * largest possible values, units is a string describing what the constraint
 * is, setter is a function that sets the time spec.
 */
typedef struct {
    int min;
    int max;
    char *units;
    ef setter;
} constraint;

static void
sminute(Evmask *mask, int min)
{
    /*
     * if (min >= 48) { mask->minutes[1] |= 0x10000 << (min-48); } else if
     * (min >= 32) { mask->minutes[1] |= 1 << (min-32); } else if (min >= 16)
     * { mask->minutes[0] |= 0x10000 << (min-16); } else { mask->minutes[0]
     * |= 1 << (min); }
     */
    if (min >= 32)
        mask->minutes[1] |= 1UL << (min - 32);
    else
        mask->minutes[0] |= 1UL << (min);
}

static void
shour(Evmask *mask, int hour)
{
    mask->hours |= 1UL << (hour);
}

static void
smday(Evmask *mask, int mday)
{
    mask->mday |= 1UL << (mday);
}

static void
smonth(Evmask *mask, int month)
{
    mask->month |= 1 << (month);
}

static void
swday(Evmask *mask, int wday)
{
    if (wday == 0)
        wday = 7;
    mask->wday |= 1 << (wday);
}

static constraint minutes = {
    0, 59, "minutes", sminute
};
static constraint hours = {
    0, 23, "hours", shour
};
static constraint mday = {
    0, 31, "day of the month", smday
};
static constraint months = {
    1, 12, "months", smonth
};
static constraint wday = {
    0, 7, "day of the week", swday
};


/*
 * check a number against a constraint.
 */
static int
constrain(int num, constraint *limit)
{
    return (num >= limit->min) || (num <= limit->max);
}


static int logicalline;


/*
 * pick a number off the front of a string, validate it, and repoint the
 * string to after the number.
 */
static int
number(char **s, constraint *limit)
{
    int num;
    char *e;

    num = strtoul(*s, &e, 10);
    if (e == *s) {
        error("line %d: badly formed %s string <%s> ", logicalline, limit ? limit->units : "time", e);
        return 0;
    }

    if (limit) {
        if (constrain(num, limit) == 0) {
            error("line %d: bad number in %s", logicalline, limit->units);
            return 0;
        }
    }
    *s = e;
    return num;
}


/*
 * assign a value to an Evmask
 */
static void
assign(int time, Evmask *mask, ef setter)
{
    (*setter) (mask, time);
}


/*
 * pick a time field off a line, returning a pointer to the rest of the line
 */
static char *
parse(char *s, cron *job, constraint *limit)
{
    int num, num2, skip;

    if (s == 0)
        return 0;

    do {
        skip = 0;
        num2 = 0;

        if (*s == '*') {
            num = limit->min;
            num2 = limit->max;
            ++s;
        } else {
            num = number(&s, limit);

            if (*s == '-') {
                ++s;
                num2 = number(&s, limit);
                skip = 1;
            }
        }

        if (*s == '/') {
            ++s;
            skip = number(&s, 0);
        }

        if (num2) {
            if (skip == 0)
                skip = 1;
            while (constrain(num, limit) && (num <= num2)) {
                assign(num, &job->trigger, limit->setter);
                num += skip;
            }
        } else
            assign(num, &job->trigger, limit->setter);

        if (isspace(*s))
            return firstnonblank(s);
        else if (*s != ',')
            return 0;
        ++s;
    } while (1);
}


/*
 * read the entire time spec off the start of a line, returning either null
 * (an error) or a pointer to the rest of the line.
 */
static char *
getdatespec(char *s, cron *job)
{
    bzero(job, sizeof *job);

    s = parse(s, job, &minutes);
    s = parse(s, job, &hours);
    s = parse(s, job, &mday);
    s = parse(s, job, &months);
    s = parse(s, job, &wday);

    return s;
}


/*
 * add a job to a crontab.
 */
static void
anotherjob(crontab *tab, cron *job)
{
    EXPAND(tab->list, tab->szl, tab->nrl);

    tab->list[tab->nrl++] = *job;
}


/*
 * copy a string or die.
 */
static char *
xstrdup(char *src, char *what)
{
    char *ret;

    if (ret = strdup(src))
        return ret;

    fatal("%s: %s", what, strerror(errno));
    return 0;
}


/*
 * add an environment variable to the job environment
 */
static void
setjobenv(crontab *tab, char *env)
{
    char *p;
    int i;

    if (env) {
        env = xstrdup(env, "setjobenv");
        if ((p = strchr(env, '=')) == 0) {
            free(env);
            return;
        }

        for (i = 0; i < tab->sze; i++)
            if (strncmp(tab->env[i], env, (p - env)) == 0) {
                free(tab->env[i]);
                tab->env[i] = env;
                return;
            }
    }

    EXPAND(tab->env, tab->sze, tab->nre);
    tab->env[tab->nre++] = env;
}


/*
 * compile a crontab
 */
int
readcrontab(crontab *tab, FILE *f)
{
    char *s;
    cron job;

    lineno = 0;
    while (1) {
        logicalline = lineno + 1;
        if ((s = fgetlol(f)) == 0)
            break;

        s = firstnonblank(s);

        if (*s == 0 || *s == '#' || *s == '\n')
            continue;

        if (isalpha(*s)) {      /* vixie-style environment variable
                                 * assignment */
            char *q;

            for (q = s; isalnum(*q); ++q)
                ;
            if (*q == '=')
                setjobenv(tab, s);
        } else if (s = getdatespec(s, &job)) {
            char *p = strchr(s, '\n');

            if (p)
                *p++ = 0;
            job.command = xstrdup(s, "readcrontab::command");
            job.input = p ? xstrdup(p, "readcrontab::input") : 0;
            anotherjob(tab, &job);
        } else {
            /* null-terminate the env[] array and we're ready to go */
            setjobenv(tab, 0);

            return 0;
        }
    }
    return 1;
}


/*
 * convert a time into an event mask
 */
void
tmtoEvmask(struct tm *tm, int interval, Evmask *time)
{
    int i;

    bzero(time, sizeof *time);
    if (interval > 1)
        tm->tm_min -= tm->tm_min % interval;

    for (i = 0; i < interval; i++)
        if (tm->tm_min + i < 60)
            sminute(time, tm->tm_min + i);

    shour(time, tm->tm_hour);
    smday(time, tm->tm_mday);
    smonth(time, 1 + tm->tm_mon);
    swday(time, tm->tm_wday);
}


/*
 * pull a random value out of the cron environment
 */
char *
jobenv(crontab *tab, char *name)
{
    int i, len = strlen(name);

    for (i = 0; i < tab->nre; ++i)
        if (strncmp(tab->env[i], name, len) == 0 && (tab->env[i][len] == '='))
            return 1 + len + tab->env[i];
    return 0;
}
