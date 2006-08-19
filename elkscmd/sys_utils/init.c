/*
 * init  A System-V init Clone.
 *
 * Usage: /bin/init
 *       init [0123456]
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

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <utmp.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <memory.h>
#include <errno.h>

/*#define DEBUG*/

#if 0
#define _(A) A
#else
#define _(A) ()
#endif

#define FLAG_RESPAWN   1
#define FLAG_WAIT      2
#define RUNLEVELS     12
#define BUFSIZE      256
#define INITTAB      "/etc/inittab"
#define INITLVL      "/etc/initlvl"
#define SHELL        "/bin/sh"
#define GETTY        "/bin/getty"
#define DEVTTY       "/dev/tty1"
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

int hash(string)
char *string;
{
	char *p;
	int result = 0, i=1;

	p = string;
	while (*p)
		result += ( (*p++)-'a')*(i++);

	return result;
}

/* `gently' exit */
#if 1
# define PANIC0 fputs("init panic\n", stderr),fflush(stderr),exit(1)
# define PANIC(a) fputs("init panic\n", stderr),fputs(a, stderr),exit(1)

#else
# define PANIC(a) exit(1)
#endif

#ifdef DEBUG
#define FPUTS(A) fputs(A,stderr);
#define FPUTC(A) fputc(A,stderr);
#define FPUTD(A) fprintf(stderr, "%d", A);
#else
#define FPUTS(A)
#define FPUTC(A)
#define FPUTD(A)
#endif

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
	pid_t pid;
};

struct tabentry
 children[MAXCHILD],
 *nextchild=children, *thisOne;
char runlevel;
char prevRunlevel;

struct utmp utentry;

void parseLine(line, foo)
const char* line;
void foo();
{
	char *a[4];
	int k = 0, action;
	char buf[256], *p;

	strcpy(buf, line);
	a[k++] = p = buf;
	while(*p <= ' '){
		if(*p == 0)return;
		p++;
	}
	if(*p == '#')return;
	while(k<4){
		/* looking for the k-th ':' */
		while(*p && *p != ':') p++;
		*p = 0;
		a[k++] = ++p;
	}
	foo(a);
}

void scanFile(foo)
void foo();
{
	int f, left;
	char buf[BUFSIZE], *line, *next;

	f = open(INITTAB, O_RDONLY);
	if(-1 == f)
		PANIC0;

	left = read(f, buf, BUFSIZE);
	line = strtok(buf, "\n");
	next = strtok(NULL, "\n");

	while (1) {
		if (!next) {
			if(line == buf)
				goto out;
			memmove(buf, line, left);
			left += read(f, buf+left, BUFSIZE-left);
			line = buf;
			next = strtok(buf, "\n");
		}
		else {
			parseLine(line, foo);

			left -= next-line;
			line = next;
			next = strtok(NULL, "\n");
		}
	}
out:
	close(f);
}

/* returns a pointer to the child or NULL */
struct tabentry * matchPid(pid)
pid_t pid;
{
	struct tabentry *i=nextchild;
	while(i!=children)
		if((--i)->pid == pid) return i;
	return 0;
}

/* returns a pointer to the child or NULL */
struct tabentry * matchId(id)
char * id;
{
	struct tabentry * i=nextchild;
	while(i!=children)
		if(!strcmp((--i)->id, id)) return i;
	return 0;
}


/* appends child information in the array */
void appendChild (id, pid)
char * id;
pid_t pid;
{
	if(MAXCHILD == nextchild-children)
		PANIC("too many children");
	memcpy(nextchild->id, id, 2);
	nextchild->id[2] = 0;
	nextchild->pid = pid;
	nextchild++;
}

/* removes child information from the array */
void removeChild (pos)
struct tabentry * pos;
{
	if(pos-children >= nextchild-children)
		PANIC("unexistent child");
	memcpy(pos, --nextchild, sizeof (struct tabentry) );
}

void doSleep(int sec)
{
/*
 struct timeval tv;

 tv.tv_sec = sec;
 tv.tv_usec = 0;

 while(select(0, NULL, NULL, NULL, &tv) < 0 && errno == EINTR)
  ;
 */
}

void makefork()
{
    int pid;
    int fd;
    char **argv;
    
    pid = fork();
    if(pid == 0){
	setsid();
#ifdef DEBUG
        close(0);
	close(1);
	close(2);
#endif
	fd = open(DEVTTY, O_RDWR);
        dup2(fd ,STDIN_FILENO);
	dup2(fd ,STDOUT_FILENO);
        dup2(fd ,STDERR_FILENO);
        argv[0] = "/bin/ps";
        argv[1] = NULL;
	execv(argv[0], argv);
    }
    while(pid != wait(NULL));
}

pid_t respawn(a)
const char **a;
{
    int pid, status;
    char *argv[4], buf[128];	
    int fd;
    char *devtty;

    FPUTS("spawning \"")
    FPUTS(a[3])
    FPUTS("\"\n")

    if (a[3] == 0)
	return 1;
    
    pid = fork();
    if (-1 == pid)
	PANIC0;
	
    if (0 == pid)
    {
	setsid();
#ifdef DEBUG
        close(0);
	close(1);
	close(2);
#endif
	strcpy(buf, a[3]);
	if(!strncmp(buf, GETTY, sizeof GETTY -1)){
	    
	    devtty = strchr(buf, ' ');

	    if(!devtty)
		PANIC0;
	    *(devtty++) = 0;
	    if ((fd = open(devtty, O_RDWR)) < 0)
		PANIC0;

	    dup2(fd ,STDIN_FILENO);
	    dup2(fd ,STDOUT_FILENO);
	    dup2(fd ,STDERR_FILENO);

	    argv[0] = GETTY;
	    argv[1] = devtty;
	    argv[2] = NULL;

	    execv(argv[0], argv);
	}	
	else
	{
	    if ((fd = open(DEVTTY, O_RDWR)) < 0)
		PANIC0;

	    dup2(fd ,STDIN_FILENO);
	    dup2(fd ,STDOUT_FILENO);
	    dup2(fd ,STDERR_FILENO);

	    argv[0] = SHELL;
	    argv[1] = "-e";
	    argv[2] = buf;
	    argv[3] = strtok(buf, " ");
	    argv[4] = NULL;

    	    execv(argv[0], argv);
	}

	PANIC0;
    }

    FPUTS("owner process owns ")
    FPUTD(pid)
    FPUTC('\n')

/* here I must do something about utmp */
    return pid;
}

void passOne(a)
char **a;
{
	pid_t pid;

	switch(hash(a[2])){
	case INITDEFAULT:
		runlevel = a[1][0];
		break;

	case SYSINIT:
		pid = respawn(a);
		while(pid != wait(NULL));
		break;

	default:
	/* ignore */
	;
	}
}

void getRunlevel(a)
char **a;
{
	if(INITDEFAULT == hash(a[2])){
		runlevel = a[1][0];
	}
}

void exitRunlevel(a)
char **a;
{
	FPUTS(a[0]) FPUTC(':') FPUTS(a[1]) FPUTC(':') FPUTS(a[2]) FPUTC(':')
	FPUTS(a[3])

	if( a[1][0] && !strchr(a[1], runlevel) ) {
		pid_t pid;
		struct tabentry *child;

		FPUTS(" stop it!")

		/* if running, terminate it gently*/
		child = matchId(a[0]);
		if(!child) {
			FPUTS(" not running\n")
			return;
		}
		if(!child->pid) {
			FPUTS(" not running\n")
			return;
		}
		kill(child->pid, SIGTERM);

		doSleep(2);                  /* give it the time */

		/* if still running, kill it right away */
		child = matchId(a[0]);
		if(!child) return;
		if(!child->pid) return;
		kill(child->pid, SIGKILL);
	}
	FPUTC('\n')
}

void enterRunlevel(a)
char **a;
{
    pid_t pid;

    FPUTS(a[0]) FPUTS(" : ") FPUTS(a[1]) FPUTS(" : ") FPUTS(a[2]) FPUTS(" : ")
    FPUTS(a[3])

    if( !a[1][0] || strchr(a[1], runlevel) ) 
    {
	int andWait=0;

  /* if not running, spawn it */
	if ( !matchId(a[0]) )
	switch(hash(a[2])){
	case WAIT:
	    andWait = 1;
	case RESPAWN:
	case ONCE:
	    pid = respawn(a);
	    if(andWait)
		while(pid != wait(NULL));
	    else
		appendChild(a[0], pid);
	    break;
	default:
	{
	    FPUTS("discarded\n")
	}
	}
	else
	{
	    FPUTS("already running!\n")
        }
    }
}

void spawnThisOne(a)
char **a;
{
	if( !strncmp(a[0], thisOne->id, 2) ) {
		switch(hash(a[2])) {
		case RESPAWN:
		case ONDEMAND:
			thisOne->pid = respawn(a);
			break;
		default:
		removeChild(thisOne);
		}
		
		strcpy(utentry.ut_line, strstr(a[3], "/tty")+1);
		time(&utentry.ut_time);
		utentry.ut_id[0] = a[0][0];
		utentry.ut_id[1] = a[0][1];
		utentry.ut_pid = thisOne->pid;
		setutent();
		pututline(&utentry);
		endutent();
	}
}

void handle_signal(sig)
int sig;
{
	FPUTS("got signaled!\n")
	switch(sig) {
		case SIGHUP:
		/* got signaled by another instance of init, change runlevel! */
		{
			prevRunlevel = runlevel;
			scanFile(getRunlevel);
	
			if (runlevel != prevRunlevel){

				/* -stop all running children not needed in new run-level */
				scanFile(exitRunlevel);

				/* -start all non running children needed in new run-level */
				scanFile(enterRunlevel);
			}
		}
		break;
	}
}

int main(argc, argv)
char ** argv;
int argc;
{
	int fd;
	pid_t pid;
	struct tabentry *entry;

	argv[0] = "init";
#ifdef DEBUG
	fd = open(DEVTTY, O_RDWR);
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
	FPUTS("entered /bin/init\n")
#endif

	memset(&utentry, 0, sizeof(struct utmp));
	utentry.ut_type = INIT_PROCESS;
	time(&utentry.ut_time);
	utentry.ut_id[0] = 'l';
	utentry.ut_id[1] = '1';
	utentry.ut_pid = getpid();
	strcpy(utentry.ut_user, "INIT");
	setutent();
	pututline(&utentry);

	/* am I the No.1 init? */
	if(getpid() == 1){
	
		/*   signal(SIGALRM,  handle_signal); */
		signal(SIGHUP,   handle_signal);
		/*   signal(SIGINT,   handle_signal); */
		/*   signal(SIGCHLD,  handle_signal); */
		/*   signal(SIGPWR,   handle_signal); */
		/*   signal(SIGWINCH, handle_signal); */
		/*   signal(SIGUSR1,  handle_signal); */
		/*   signal(SIGSTOP,  handle_signal); */
		/*   signal(SIGTSTP,  handle_signal); */
		/*   signal(SIGCONT,  handle_signal); */
		/*   signal(SIGSEGV,  handle_signal); */			
	
		setutent();

		/* get runlevel & spawn sysinit */
		FPUTS("scanfile - passOne\n")
		scanFile(passOne);
		FPUTS("entered runlevel ")
		FPUTC(runlevel)
		FPUTC('\n')
	  
		/* spawn needed children */
		FPUTS("scanfile - enterRunlevel\n")
		scanFile(enterRunlevel);	

		endutent();

		/* wait for signals. */
		while(1) {	

			FPUTS("about to go waiting...\n")
			pid = wait(NULL);

			FPUTS("and ")
			FPUTD(pid)
			FPUTS(" came out\n")

			if(-1 == pid)
				continue;

			FPUTS("child ")
			FPUTD(pid)
			FPUTS(" died...\n")

			thisOne = matchPid(pid);
			if(!thisOne)
				continue;

			FPUTS("scanfile - spawnThisOne\n")
			scanFile(spawnThisOne);
		}
	}
	else
	{
		/* store the new run-level into /etc/initrunlvl */
		int f = open(INITLVL, O_WRONLY);
		write(f, argv[1], 1);
		close(f);

		FPUTS("change request to ")
		FPUTC(argv[1][0])
		FPUTC('\n')

		/* signal (SIGHUP) the No.1 init that we must switch run-level. */
		kill(1, SIGHUP);
	}
	return 0;
}

