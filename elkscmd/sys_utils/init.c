/*
 * init  A System-V init Clone.
 *
 * Usage: /bin/init
 *       init [0123456]
 *
 * Apr 2020 Greg Haerr
 *	Many more bug fixes.
 *
 * 2001-08-23  Harry Kalogirou
 *	Many bug fixes. In general made it finaly work.
 *	Also corrected the C formating.
 *
 * 1999-11-07  mario.frasca@home.ict.nl
 *
 *  Copyright 1999 Mario Frasca
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <utmp.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <paths.h>
#include <limits.h>

#define USE_UTMP	0	/* =1 to use /var/run/utmp*/
#define DEBUG		0	/* =1 for debug messages*/

/* debug and sysinit/respawn sh -e output goes here*/
#define CONSOLE       _PATH_CONSOLE

#if !DEBUG
#define debug(...)
#define debug2(...)
#endif

#define BUFSIZE      256
#define INITTAB      _PATH_INITTAB
#define INITLVL      _PATH_INITLVL
#define SHELL        _PATH_BSHELL
#define GETTY        _PATH_GETTY
#define ENV_SYNC     "sync"		/* sync= /bootopts env var override */
#define SYNC_DEFAULT 0			/* default sync timer */
#define MAXCHILD     8

/* 'hashed' strings */
#define RESPAWN      362
#define WAIT         122
#define ONCE          62
#define BOOT         147
#define BOOTWAIT     465
#define POWERFAIL    403
#define POWERFAILNOW 951
#define POWERWAIT    577
#define POWEROKWAIT  829
#define CTRLALTDEL   504
#define OFF           39
#define ONDEMAND     240
#define INITDEFAULT  707
#define SYSINIT      398
#define KBREQUEST    622

/*
  each of the entries from inittab corresponds to a child, each of which:
  has a unique 2 chars identifier.
  is allowed to run in some run-levels.
  might need to be waited for completion when spawned.
  might be running or not (pid different from zero).
  might have to been respawned.
  was respawned at a certain point in time.

  for each running child, we keep the pid assigned to a given id.
  We take this information each time from the INITTAB file.
*/
struct tabentry {
	char id[3];
	char norespawn;			/* don't respawn if fails within 3 seconds*/
	pid_t pid;
	time_t starttime;		/* start time of first spawn*/
};

static char *initname;			/* full path to this program*/
static struct tabentry children[MAXCHILD], *nextchild=children, *thisOne;
static char runlevel;
static char prevRunlevel;
static char nosysinit;
static int sync_interval = SYNC_DEFAULT;
#if USE_UTMP
static struct utmp utentry;
#endif

/* Print an error message and die*/
static void fatalmsg(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	fprintf(stderr, "%s: ", initname);
	vfprintf(stderr, msg, args);
	va_end(args);
	exit(1);
}

#if DEBUG
static void debug(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	fprintf(stderr, "%s: ", initname);
	vfprintf(stderr, msg, args);
	va_end(args);
}

static void debug2(const char *str)
{
	fprintf(stderr, "%s", str);
}
#endif


/* special handling to decrease stdio buffer sizes for ELKS*/
static unsigned char bufin[1];
FILE  stdin[1] =
{
   {
    bufin,
    bufin,
    bufin,
    bufin,
    bufin + sizeof(bufin),
    0,
    _IOFBF | __MODE_READ | __MODE_IOTRAN
   }
};

static unsigned char bufout[64];
FILE  stdout[1] =
{
   {
    bufout,
    bufout,
    bufout,
    bufout,
    bufout + sizeof(bufout),
    1,
    _IOFBF | __MODE_WRITE | __MODE_IOTRAN
   }
};

static unsigned char buferr[64];
FILE  stderr[1] =
{
   {
    buferr,
    buferr,
    buferr,
    buferr,
    buferr + sizeof(buferr),
    2,
    _IONBF | __MODE_WRITE | __MODE_IOTRAN
   }
};

void parseLine(const char* line, void func())
{
	int k = 0;
	char *p, *a[5], buf[BUFSIZE];

	strcpy(buf, line);
	a[k++] = p = buf;
	while (*p <= ' ') {
		if (*p == 0) return;
		p++;
	}
	if (*p == '#') return;
	while (k < 5) {
		/* looking for the k-th ':' */
		while (*p && *p != ':') p++;
		if (*p == 0) {
			*p = 0;
			a[k++] = NULL;
			continue;
		}
		*p = 0;
		a[k++] = ++p;
	}
#if DEBUG
	for (k=0; k<5; k++) {
		debug("a[%d]='%s'\n", k, a[k]);
	}
#endif
	func(a);
}

void scanFile(void func())
{
	char buf[BUFSIZE+1];
	FILE *fp;

	alarm(0);	/* eliminate possibility of interrupted fgets read */
	fp = fopen(INITTAB, "r");
	if (!fp) fatalmsg("Missing %s\r\n", INITTAB);

	while (fgets(buf, BUFSIZE, fp)) {
		if (buf[strlen(buf)-1] == '\n')
			buf[strlen(buf)-1] = '\0';
		debug("[buf='%s',%d]\n", buf, strlen(buf));
		parseLine(buf, func);
	}
	fclose(fp);
	alarm(sync_interval);
}

/* returns a pointer to the child or NULL */
struct tabentry *matchPid(pid_t pid)
{
	struct tabentry *i=nextchild;
	while (i!=children)
		if ((--i)->pid == pid) return i;
	return 0;
}

/* returns a pointer to the child or NULL */
struct tabentry *matchId(const char *id)
{
	struct tabentry *i = nextchild;
	while (i!=children)
		if (!strcmp((--i)->id, id)) return i;
	return 0;
}


/* add new child entry in the array */
struct tabentry *newChild (const char *id)
{
	if (MAXCHILD == nextchild-children)
		fatalmsg("too many children\r\n");
	memcpy(nextchild->id, id, 2);
	nextchild->id[2] = 0;
	nextchild->pid = 0;
	nextchild->starttime = 0;;
	nextchild->norespawn = 0;
	nextchild++;
	return (nextchild - 1);
}

/* removes child information from the array */
void removeChild(struct tabentry *pos)
{
	if (pos-children >= nextchild-children)
		fatalmsg("nonexistent child\r\n");
	memcpy(pos, --nextchild, sizeof (struct tabentry));
}

pid_t respawn(const char **a)
{
    int pid;
    char *argv[5], buf[PATH_MAX];
    int fd;
    char *devtty;

    if (a[3] == NULL || a[3][1] == '\0') return -1;

    pid = fork();
    if (pid == -1) fatalmsg("No fork (%d)\r\n", errno);
    if (pid) debug("spawning %d '%s'\r\n", pid, a[3]);

    if (0 == pid) {
#if DEBUG
	usleep(100000L);	/* allow init messages prior to ours*/
#endif
	setsid();
	strcpy(buf, a[3]);
	if (!strncmp(buf, GETTY, sizeof(GETTY)-1)) {
	    char *baudrate;
	    devtty = strchr(buf, ' ');

	    if (!devtty) fatalmsg("Bad getty line: '%s'\r\n", buf);
	    *devtty++ = 0;
	    baudrate = strchr(devtty, ' ');
	    if (baudrate)
		*baudrate++ = 0;
	    if ((fd = open(devtty, O_RDWR)) < 0)
			fatalmsg("Can't open %s (errno %d)\r\n", devtty, errno);

	    argv[0] = GETTY;
	    argv[1] = devtty;
	    argv[2] = baudrate;
	    argv[3] = NULL;
	    debug("execv '%s' '%s' '%s'\r\n", argv[0], argv[1], argv[2]);

	    dup2(fd ,STDIN_FILENO);
	    dup2(fd ,STDOUT_FILENO);
	    dup2(fd ,STDERR_FILENO);
	    if (fd > STDERR_FILENO)
			close(fd);
	    for (fd = 3; fd < NR_OPEN; fd++)
			close(fd);
	    execv(argv[0], argv);
	}
	else
	{
	    if ((fd = open(CONSOLE, O_RDWR)) < 0)
		fatalmsg("Can't open %s (errno %d)\r\n", CONSOLE, errno);

	    argv[0] = SHELL;
	    argv[1] = "-c";
	    argv[2] = buf;
	    argv[3] = NULL;
	    debug("execv '%s' '%s' '%s'\r\n", argv[0], argv[1], argv[2]);

	    dup2(fd ,STDIN_FILENO);
	    dup2(fd ,STDOUT_FILENO);
	    dup2(fd ,STDERR_FILENO);
	    if (fd > STDERR_FILENO)
			close(fd);
	    for (fd = 3; fd < NR_OPEN; fd++)
			close(fd);
	    execv(argv[0], argv);
	}

	fatalmsg("Exec failed: %s (errno %d)\r\n", argv[0], errno);
    }

/* here I must do something about utmp */
    return pid;
}

int hash(const char *string)
{
	const char *p;
	int result = 0, i=1;

	if (string == NULL) return 0;
	p = string;
	while (*p)
		result += ((*p++)-'a')*(i++);

	return result;
}

void passOne(const char **a)
{
	pid_t pid;

	switch (hash(a[2])) {
	case INITDEFAULT:
		runlevel = a[1][0];
		break;

	case SYSINIT:
		if (!nosysinit) {
			pid = respawn(a);
			while (pid != wait(NULL));
		}
		break;

	default:
	/* ignore */
		;
	}
}

void getRunlevel(char **a)
{
	if (INITDEFAULT == hash(a[2])) runlevel = a[1][0];
}

void exitRunlevel(char **a)
{
	debug("EXITRUNLVL %s:%s:%s:%s ", a[0], a[1], a[2], a[3]);

	if (a[1][0] && !strchr(a[1], runlevel)) {
		struct tabentry *child;

		debug2("stop ");

		/* if running, terminate it gently */
		child = matchId(a[0]);
		if (!child) {
			debug2("not running\r\n");
			return;
		}
		if (!child->pid) {
			debug2("not running\r\n");
			return;
		}
		kill(child->pid, SIGTERM);
		usleep(500000L);			/* give it half a sec*/

		/* if still running, kill it right away */
		child = matchId(a[0]);
		if (!child) return;
		if (!child->pid) return;
		kill(child->pid, SIGKILL);
		child->pid = 0;
	}
	debug2("\r\n");
}

void enterRunlevel(const char **a)
{
	struct tabentry *child;
	pid_t pid;

	debug("ENTERUNLVL %s:%s:%s:%s ", a[0], a[1], a[2], a[3]);

	if (!a[1][0] || strchr(a[1], runlevel)) {
		int andWait = 0;

		/* if not running, spawn it */
		child = matchId(a[0]);
		if (!child || !child->pid) {
			switch (hash(a[2])) {
			case WAIT:
				andWait = 1;
			case RESPAWN:
			case ONCE:
				debug2("\r\n");
				pid = respawn(a);
				if (andWait) while (pid != wait(NULL));
				else {
					if (!child)
						child = newChild(a[0]);
					if (child) {
						child->pid = pid;
						child->starttime = time(NULL);
						child->norespawn = 0;
					}
				}
				break;
			default:
				debug2("discarded\r\n");
			}
		} else debug2("already running\r\n");
	}
}

void spawnThisOne(const char **a)
{
	if (!strncmp(a[0], thisOne->id, 2)) {
	  if (!a[1][0] || strchr(a[1], runlevel)) {
		switch (hash(a[2])) {
		case RESPAWN:
		case ONDEMAND:
			if (time(NULL) - thisOne->starttime < 3) {
				debug("Respawn timeout on pid %d, stopping\r\n", thisOne->pid);
				thisOne->norespawn = 1;
			}
			if (!thisOne->norespawn)
				thisOne->pid = respawn(a);
			break;
		default:
			removeChild(thisOne);
		}
#if USE_UTMP
		strcpy(utentry.ut_line, strstr(a[3], "/tty")+1);
		time(&utentry.ut_time);
		utentry.ut_id[0] = a[0][0];
		utentry.ut_id[1] = a[0][1];
		utentry.ut_pid = thisOne->pid;
		setutent();
		pututline(&utentry);
		endutent();
#endif
	  }
	}
}

void handle_signal(int sig)
{
	int fd;
	char c;

	debug("signaled %d\r\n", sig);
	switch(sig) {
	case SIGHUP:
		signal(SIGHUP, handle_signal);

		/* got signaled by another instance of init, change runlevel! */
		fd = open(INITLVL, O_RDONLY);
		if (fd < 0) {
			debug("open %s failed (%d)\r\n", INITLVL, errno);
			return;
		}
		if (read(fd, &c, 1) != 1) {
			debug("read %s failed (%d)\r\n", INITLVL, errno);
			close(fd);
			return;
		}
		close(fd);
		prevRunlevel = runlevel;
		runlevel = c;
		debug("previous runlevel %c, new runlevel %c\r\n", prevRunlevel, runlevel);
		if (runlevel != prevRunlevel) {
			/* stop all running children not needed in new run-level */
			debug("SCAN exitrunlevel\r\n");
			scanFile(exitRunlevel);
			/* start all non running children needed in new run-level */
			debug("SCAN start runlevel %c\r\n", runlevel);
			scanFile(enterRunlevel);
		}
		break;
	case SIGALRM:
		signal(SIGALRM, handle_signal);
		debug("SYNC\r\n");
		sync();
		alarm(sync_interval);
		break;
	}
}

int main(int argc, char **argv)
{
	pid_t pid;
	int ac = argc;
	char **av = argv;
	char *p;

//	argv[0] = "init";
	initname = argv[0];
#if DEBUG
	int fd = open(CONSOLE, O_RDWR);
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
	debug("starting\r\n");

	/* debug /bootopts:*/
	printf("ARGS: ");
	char **av2 = argv;
	while (*av2)
		printf("'%s'", *av2++);
	printf("\n");
	printf("ENV: ");
	char **env = environ;
	while (*env)
		printf("'%s'", *env++);
	printf("\n");
#endif

#if USE_UTMP
	memset(&utentry, 0, sizeof(struct utmp));
	utentry.ut_type = INIT_PROCESS;
	time(&utentry.ut_time);
	utentry.ut_id[0] = 'l';
	utentry.ut_id[1] = '1';
	utentry.ut_pid = getpid();
	strcpy(utentry.ut_user, "INIT");
	setutent();
	pututline(&utentry);
#endif
	/* am I the No.1 init? */
	if (getpid() == 1) {
		signal(SIGHUP, handle_signal);
		signal(SIGALRM, handle_signal);
#if USE_UTMP
		setutent();
#endif
		while (ac > 1) {
			if (av[1][0] == 'n')
				nosysinit = 1;
			ac--;
			av++;
		}

		/* get runlevel & spawn sysinit */
		debug("SCAN sysinit\r\n");
		scanFile(passOne);

		if (argc > 1 && argv[1][0] >= '0' && argv[1][0] <= '9')
			runlevel = argv[1][0];

		/* spawn needed children */
		debug("SCAN start runlevel %c\r\n", runlevel);
		scanFile(enterRunlevel);
#if USE_UTMP
		endutent();
#endif
#if !DEBUG
		close(0);
		close(1);
		close(2);
#endif
		/* setup buffer auto-sync using /bootopts env var */
		if ((p = getenv(ENV_SYNC)) != NULL)
			sync_interval = atoi(p);
		alarm(sync_interval);

		/* endless loop waiting for signals or child exit*/
		while (1) {
			pid = wait(NULL);
			debug("wait child exit %d\r\n", pid);
			if (pid == -1) continue;

			thisOne = matchPid(pid);
			if (!thisOne) continue;

			debug("SCAN respawn child exit\r\n");
			scanFile(spawnThisOne);
		}
	} else {
		if (argc != 2)
			fatalmsg("Usage: init <runlevel>\n");

		/* try to store new run-level into /etc/initrunlvl*/
		int f = open(INITLVL, O_CREAT|O_WRONLY, 0644);
		if (f < 0)
			debug("open '%s' failed\r\n", INITLVL);
		if (write(f, argv[1], 1) < 0)
			debug("write '%s' failed (%d)\r\n", INITLVL, errno);
		close(f);

		debug("change to runlevel %c\r\n", argv[1][0]);

		/* signal the first init to switch run level*/
		kill(1, SIGHUP);
	}
	return 0;
}
