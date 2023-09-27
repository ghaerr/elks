/*
 * run a job, mail output (if any) to the user
 */
#include <stdio.h>
#include <pwd.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <dirent.h>
#include <errno.h>
#include <paths.h>
#include <signal.h>
#ifdef ELKS
#include <linuxmt/fs.h>
#endif
#include "cron.h"

extern char *pgm;


/*
 * the child process that actually runs a job
 */
static void
runjobprocess(crontab *tab, cron *job, struct passwd *usr)
{
    pid_t jpid;
    char *home = usr->pw_dir ? usr->pw_dir : "/tmp";
    int status;
    int input[2];
    char *shell;
    input[0] = input[1] = 0;

#if TEST
    printf("cron: running '%s'\n", job->command);
#endif
    if ((shell = jobenv(tab, "SHELL")) == 0)
        shell = _PATH_BSHELL;

    setsid();
    signal(SIGCHLD, SIG_DFL);

    if (setregid(usr->pw_gid, usr->pw_gid) == -1)
        fatal("setregid(%d,%d): %s", usr->pw_gid, usr->pw_gid, strerror(errno));
    if (setreuid(usr->pw_uid, usr->pw_uid) == -1)
        fatal("setreuid(%d,%d): %s", usr->pw_uid, usr->pw_uid, strerror(errno));

    if (chdir(home) == -1)
        fatal("chdir(\"%s\"): %s", home, strerror(errno));

    if ((jpid = fork()) == -1)
        fatal("job fork(): %s", strerror(errno));
    else if (jpid == 0) {       /* the job to run */
        info("(%s) CMD (%s)", usr->pw_name, job->command);

        setsid();

        close(input[1]);
        close(0);
        dup2(input[0], 0);
        close(input[0]);
        close(2);
        dup2(1, 2);

        execle(shell, "sh", "-c", job->command, (char *)0, tab->env);
        perror(shell);
        exit(1);
    } else {                    /* runjobprocess() */
        close(input[0]);

        if (job->input && (fork() == 0)) {
            write(input[1], job->input, strlen(job->input));
            close(input[1]);
            exit(0);
        }
        close(input[1]);

#ifdef _PATH_MAIL //no email support in ELKS yet
        if (fork() == 0) {
            if (recv(output[0], peek, 1, MSG_PEEK) == 1) {
                close(0);
                dup2(output[0], 0);
                if ((to = jobenv(tab, "MAILTO")) == 0)
                    to = usr->pw_name;
                snprintf(subject, sizeof subject,
                         "Cron <%s> %s", to, job->command);
                execle(_PATH_MAIL, "mail", "-s", subject, to, (char *)0, tab->env);
                fatal("can't exec(\"%s\"): %s", _PATH_MAIL, strerror(errno));
            }
            exit(0);
        }
#endif
        /* wait for all subprocesses to finish */
        while (wait(&status) != -1)
            continue;
    }
    exit(0);
}


/*
 * validate the user and fork off a childprocess to run the job
 */
void
runjob(crontab *tab, cron *job)
{
    pid_t pid;
    struct passwd *pwd;

    if ((pwd = getpwuid(tab->user)) == 0) {
        /*
         * if we can't find a password entry for this crontab, touch mtime so
         * that it will be rescanned the next time cron checks the CRONDIR
         * for new crontabs.
         */
        error("no password entry for user %d\n", tab->user);
        time(&tab->mtime);
        return;
    }

#if DEBUG
    printtrig(&job->trigger);
    printf("%s", job->command);
    if (job->input)
        printf("<< \\EOF\n%sEOF\n", job->input);
    else
        putchar('\n');
#endif
    if ((pid = fork()) == -1)
        error("fork() in runjob: %s", strerror(errno));
    else if (pid == 0) {
        endpwent();
        runjobprocess(tab, job, pwd);
        exit(0);
    }
}
