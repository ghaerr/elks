/*
 * file:	 login.c
 * descripton:	 login into a user account
 * author:	 Alistair Riddoch <ajr@ecs.soton.ac.uk>
 * modification: David Murn <scuffer@hups.apana.org.au>
 *		 Added password entry
 * modification: Shane Kerr <kerr@wizard.net>
 *		 More work on password entry
 * modification: Al Riddoch <ajr@ecs.soton.ac.uk>  31 Jan 1999
 *		 Added utmp handling, and invocation by getty.
 */

/* todo:  add a timeout for serial and network logins */
/*	  need a signal mechanism (i.e. alarm() and SIGALRM) */

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <utmp.h>
#include <limits.h>
#include <signal.h>

/*#define USE_UTMP*/	/* Disabled until we fix the "utmp file corrupt" */
			/* issue. 17/4/2002 Harry Kalogirou */

#define STR_SIZE (PATH_MAX + 7)

void login(register struct passwd *pwd, struct utmp *ut_ent)
{
    int envno = 0;
    char **newenv;
    char user_env[STR_SIZE];
    char shell_env[STR_SIZE];
    char home_env[STR_SIZE];
    char sh_name[STR_SIZE];
    char *renv[16];

    endpwent();
    if (fchown(0, pwd->pw_uid, pwd->pw_gid) < 0) perror("login (chown)");
#ifdef USE_UTMP
    ut_ent->ut_type = USER_PROCESS;
    strncpy(ut_ent->ut_user, pwd->pw_name, UT_NAMESIZE);
    time(&ut_ent->ut_time);
    pututline(ut_ent);
    endutent();
#endif
    if (setgid(pwd->pw_gid) < 0) {
        write(STDOUT_FILENO, "setgid failed\n", 14);
        exit(1);
    }
    if (setuid(pwd->pw_uid) < 0) {
        write(STDOUT_FILENO, "setuid failed\n", 14);
        exit(1);
    }

    /* copy capitalized /bootopts env vars to login environment */
    newenv = renv;
    while (*environ) {
        if (*environ[0] >= 'A' && *environ[0] <= 'Z')
            newenv[envno++] = *environ;
        environ++;
    }

    strcpy(user_env,"USER=");
    strncpy(user_env + 5,pwd->pw_name, STR_SIZE - 6);
    newenv[envno++] = user_env;

    strcpy(home_env,"HOME=");
    strncpy(home_env + 5,pwd->pw_dir, STR_SIZE - 6);
    newenv[envno++] = home_env;

    strcpy(shell_env,"SHELL=");
    strncpy(shell_env + 6,pwd->pw_shell, STR_SIZE - 7);
    newenv[envno++] = shell_env;
    newenv[envno++] = "TERM=ansi";
    newenv[envno] = NULL;

    *sh_name = '-';
    strncpy(sh_name + 1,pwd->pw_shell, STR_SIZE - 1);

    if (chdir(pwd->pw_dir) < 0) {
        write(STDOUT_FILENO, "No home directory. Starting in /\n", 33);
        chdir("/");
    }

    environ = renv;
    execl(pwd->pw_shell, sh_name, (char*)0);

    write(STDOUT_FILENO, "No shell (errno ", 16);
    write(STDOUT_FILENO, itoa(errno), 2);
    write(STDOUT_FILENO, ")\n", 2);
    exit(1);
}

int main(int argc, char **argv)
{
    struct passwd *pwd;
    struct utmp *entryp = NULL;
    char lbuf[64], *pbuf, salt[3];

    signal(SIGTSTP, SIG_IGN);		/* ignore ^Z stop signal*/
    for (;;) {
        if (argc == 1) {
            ssize_t n;
            write(STDOUT_FILENO, "login: ", 7);
            if ((n = read(STDIN_FILENO, lbuf, sizeof(lbuf) - 1)) < 1) {
                if (errno == EINTR)
                    continue;
                exit(1);
            }
            if (lbuf[n - 1] == '\n')
                n--;
            lbuf[n] = '\0';
        } else {
            strncpy(lbuf, argv[1], UT_NAMESIZE);
            lbuf[UT_NAMESIZE] = '\0';
            argc = 1;
        }
        pwd = getpwnam(lbuf);
#ifdef USE_UTMP
        struct utmp entry;
        struct utmp newentry;
        char *tty_name;

        if ((tty_name = ttyname(0)) == NULL) {
            goto not_tty;
        }
        entry.ut_type = INIT_PROCESS;
        entry.ut_id[0] = *(tty_name + 8);
        entry.ut_id[1] = *(tty_name + 9);
        if ((entryp = getutid(&entry)) == NULL) {
            write(STDERR_FILENO, "utmp file corrupt\n", 18);
            endutent();
            exit(1);
        }
        newentry = *entryp;
        entryp = &newentry;
        entryp->ut_type = LOGIN_PROCESS;
        strncpy(entryp->ut_user, lbuf, UT_NAMESIZE);
        time(&entryp->ut_time);
        pututline(entryp);
        endutent();
not_tty:
#endif
        if ((pwd != NULL) && (pwd->pw_passwd[0] == 0)) {
            login(pwd, entryp);
        }
        pbuf = getpass("Password:");
        write(STDOUT_FILENO, "\n", 1);
        /* Must have 2 bytes of salt, else is locked */
        if (pwd != NULL && pwd->pw_passwd[1] != 0) {
            salt[0] = pwd->pw_passwd[0];
            salt[1] = pwd->pw_passwd[1];
            salt[2] = 0;
            if (!strcmp(crypt(pbuf, salt), pwd->pw_passwd)) {
                login(pwd, entryp);
            }
        }
        write(STDOUT_FILENO, "Login incorrect\n\n", 17);
    }
}
