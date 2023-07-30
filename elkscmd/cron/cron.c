/*
 * cron: run jobs at scheduled times.
 */
#include <stdio.h>
#include <pwd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <paths.h>
#include <sys/times.h>

#include "cron.h"

char *pgm;

/*
 * keep all the crontabs in a sorted-by-uid array,
 */
crontab *tabs = 0;
int nrtabs = 0;
int sztabs = 0;

static int ct_add, ct_update, ct_inact;


/*
 * SIGCHLD handler -- eat a child status, reset the signal handler, then
 * return.
 */
static void
eat(int sig)
{
    int status;
    (void)sig;

    while (waitpid(-1, &status, 0) != -1)
        continue;
    signal(SIGCHLD, eat);
}


/*
 * sort crontab entries by userid
 */
int
cmptabs(const void *c1, const void *c2)
{
    crontab *a = (crontab *) c1, *b = (crontab *) c2;

    return a->user - b->user;
}


/*
 * construct an environment entry
 */
static char *
environment(char *who, char *what)
{
    char *ptr = xrealloc(0, strlen(who) + strlen(what) + 2, 1);

    sprintf(ptr, "%s=%s", who, what);
    return ptr;
}


/*
 * validate and read in a crontab.  Checks that the crontab is named for an
 * existing user, that it's owned by root, that it's not readable or writable
 * by anyone except root, and that it's a regular file.
 */
void
process(char *file)
{
    struct stat st;
    FILE *f;
    struct passwd *user;
    crontab tab, *ent;

    if (stat(file, &st) != 0)
        return;

    if ((user = getpwnam(file)) == 0)
        return;

    if (st.st_uid != 0) {
        error("crontab for %s is not owned by root", file);
        return;
    }

    /*
     * if ( st.st_mode & (S_IWOTH|S_IWGRP|S_IROTH|S_IRGRP) ) { error("bad
     * permissions for crontab %s", file); return; }
     *
     * if ( !S_ISREG(st.st_mode) ) { error("crontab for %s in not a regular
     * file", file); return; }
     */

    /*
     * see if a crontab owned by this user already exists;  if one does, we
     * can reuse it instead of expanding the table for a new one.
     */
    tab.user = user->pw_uid;

    ent = bsearch_c(&tab, tabs, nrtabs, sizeof tabs[0], cmptabs);

    if (ent) {
        if (ent->mtime == st.st_mtime) {
            if (ent->nrl)
                ent->flags |= ACTIVE;
            return;
        }
        zerocrontab(ent);
        ct_update++;
    } else {
        EXPAND(tabs, sztabs, nrtabs);
        ent = &tabs[nrtabs++];
        bzero(ent, sizeof tabs[0]);
        ent->user = user->pw_uid;
        ct_add++;
    }

    if (f = fopen(file, "r")) {
        /*
         * preload the environment with the settings we absolutely positively
         * need.
         */
        EXPAND(ent->env, ent->sze, ent->nre + 5);
        ent->env[ent->nre++] = environment("HOME", user->pw_dir);
        ent->env[ent->nre++] = environment("USER", user->pw_name);
        ent->env[ent->nre++] = environment("LOGNAME", user->pw_name);
        ent->env[ent->nre++] = environment("PATH", _PATH_DEFPATH);
        ent->env[ent->nre++] = environment("SHELL", _PATH_BSHELL);

        readcrontab(ent, f);
        if (ent->nrl)
            ent->flags |= ACTIVE;
        ent->mtime = st.st_mtime;
        fclose(f);
    } else
        error("can't open crontab for %s: %s", file, strerror(errno));
}


/*
 * verify that the CRONDIR is a legitimate path
 */
static void
securepath(char *path)
{
    char *line = alloca(strlen(path) + 1);
    char *p, *part;
    struct stat sb, sb1;

    if (line == 0)
        fatal("%s: %m", path, strerror(errno));
    strcpy(line, path);

    /* check each part of the path */
    while ((p = strrchr(line, '/'))) {
        *p = 0;

        part = line[0] ? line : "/";

        if (stat(part, &sb) != 0)
            fatal("%s: can't stat", part);
        if (sb.st_uid != 0)
            fatal("%s: not user 0", part);
        if (sb.st_gid != 0)
            fatal("%s: not group 0", part);
        if (!S_ISDIR(sb.st_mode))
            fatal("%s: not a directory", part);
#if 0
        if (sb.st_mode & (S_IWGRP | S_IWOTH))
            fatal("%s: group or world writable", part);
#endif
    }
    if (stat(path, &sb) != 0)
        fatal("%s: can't stat", path);
    if (stat(".", &sb1) != 0)
        fatal("%s: can't stat", ".");
    if ((sb.st_dev != sb1.st_dev) || (sb.st_ino != sb1.st_ino))
        fatal("cwd not %s", _PATH_CRONDIR);
}


/*
 * does the current time fall inside a job event mask?
 */
int
triggered(Evmask *t, Evmask *m)
{
    if (((t->minutes[0] & m->minutes[0]) || (t->minutes[1] & m->minutes[1]))
        && (t->hours & m->hours)
        && (t->mday & m->mday)
        && (t->month & m->month)
        && (t->wday & m->wday))
        return 1;
    return 0;
}


/*
 * print the contents of a crontab
 */
#if DEBUG
static void
printcrontab()
{
    int i, j;

    for (i = 0; i < nrtabs; i++) {
        if (tabs[i].nrl) {
            struct passwd *pwd = getpwuid(tabs[i].user);

            if (pwd) {
                printf("crontab for %s (id %d.%d):\n", pwd->pw_name, pwd->pw_uid, pwd->pw_gid);
                for (j = 0; j < tabs[i].nrl; j++) {
                    printf("    ");
                    printtrig(&tabs[i].list[j].trigger);
                    printf("%s", tabs[i].list[j].command);
                    if (tabs[i].list[j].input)
                        printf("<< \\EOF\n%s\nEOF\n", tabs[i].list[j].input);
                    else
                        putchar('\n');
                }
            }
        }
    }
}
#endif


/*
 * search CRONDIR looking for new and changed crontabs
 */
void
scanctdir()
{
    DIR *d;
    struct dirent *ent;
    int i;

    for (i = 0; i < nrtabs; i++)
        tabs[i].flags &= ~ACTIVE;

    ct_add = ct_update = ct_inact = 0;

    if (d = opendir(".")) {
        while (ent = readdir(d))
            process(ent->d_name);
        closedir(d);
        qsort(tabs, nrtabs, sizeof tabs[0], cmptabs);
    }
    for (i = 0; i < nrtabs; i++)
        if (!(tabs[i].flags & ACTIVE))
            ct_inact++;
}


/*
 * punt ourselves into the background
 */
void
daemonize()
{
#if !(DEBUG)
    pid_t pid = fork();

    if (pid == -1)
        fatal("backgrounding: %s", strerror(errno));
    if (pid != 0)
        exit(0);
    freopen("/dev/null", "r", stdin);
    //freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
    setsid();
    pid = fork();
    if (pid == -1)
        fatal("double-fork: %s", strerror(errno));
    if (pid != 0)
        exit(0);
    interactive = 0;
#endif
}


time_t ct_dirtime;


/*
 * see if the crondir has been modified since the last time we looked at it.
 * If it has, update our crontab collection.
 */
void
checkcrondir()
{
    time_t newtime = mtime(".");

    if (newtime != ct_dirtime) {
        scanctdir();
#if DEBUG
        printf("scanctdir:  time WAS %s", ctime(&ct_dirtime));
        printf("                  IS %s", ctime(&newtime));
        printf("total %d, added %d, updated %d, active %d\n",
               nrtabs, ct_add, ct_update, nrtabs - ct_inact);
        printcrontab();
#endif
        ct_dirtime = newtime;
    }
}


/*
 * cron.
 */
int
main(int argc, char **argv)
{
    time_t ticks;
    Evmask Now;
    int i, j;
    int interval = 1;
    unsigned int delay;

    pgm = argv[0];
    if (argc > 1)
        interval = atoi(argv[1]);

    if (interval > 60)
        fatal("there are only 60 minutes to the hour.");
    else if (interval < 1)
        interval = 1;

    if (xis_crondir() != 0) {
        printf("No '%s' directory - terminating\n", _PATH_CRONDIR);
        exit(1);
    }

    xchdir(_PATH_CRONDIR);

    securepath(_PATH_CRONDIR);

    ct_dirtime = 0;

    daemonize();

    info("startup");

    signal(SIGCHLD, eat);
    signal(SIGHUP, SIG_IGN);

    while (1) {
        struct tm *tm;
        int adjust;

#if TEST
        unsigned int n = 2;
        do {
            printf("cron: sleep start %d\n", n);
            n = sleep(n);
            printf("cron: sleep return %d\n", n);
        } while (n > 0);
#else
        time(&ticks);
        tm = localtime(&ticks);
        adjust = (30 - tm->tm_sec) % 60;
        /*
         * if ( adjust ) error("adjust: %d", adjust);
         */
        if (adjust < -15 || adjust > 15)
            error("adjust: %d", adjust);

        delay = (60 * interval) + adjust;
        if (delay > 300)
            error("long delay: %d", delay);

        while (delay) {
            unsigned int left = sleep(delay);
            if (left > delay) {
                error("sleep(%d) returned %d", delay, left);
                //--delay;
            } else
                delay = left;
        }
#endif

        checkcrondir();

        time(&ticks);
        tm = localtime(&ticks);
        tmtoEvmask(tm, interval, &Now);
#if DEBUG
        printf("run Evmask:");
        printtrig(&Now);
        putchar('\n');
#endif

        for (i = 0; i < nrtabs; i++) {
            for (j = 0; j < tabs[i].nrl; j++) {
#if !TEST
                if ((tabs[i].flags & ACTIVE) && triggered(&Now, &(tabs[i].list[j].trigger)))
#endif
                {
                    runjob(&tabs[i], &tabs[i].list[j]);
                }
            }
        }
    }
}
