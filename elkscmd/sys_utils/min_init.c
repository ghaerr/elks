/*
 * min_init.c
 *
 * This is a small version of init for use in the ELKS project.
 * It is not fully functional, and may not be the most efficient
 * implementation for larger systems. It minimises memory usage and
 * code size.
 *
 * Copyright 1997 Alistair Riddoch
 * ajr@ecs.soton.ac.uk
 *
 * This file may be distributed under the terms of the GNU General Public
 * License v2, or at your option any later version.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

#define USE_UTMP	0	/* =1 to use /var/run/utmp*/
#define DEBUG		0	/* =1 for debug messages*/

#if USE_UTMP
#include <utmp.h>
struct utmp entry;
#endif

struct exec_struct {
	char tty_name[10];
	pid_t tty_pid;
};

#define NUM_TTYS    1
struct exec_struct tty_list[] = {
	{"/dev/tty1", 0},
//  {"/dev/tty2", 0},
//  {"/dev/tty3", 0},
//	{"/dev/ttyS1", 0},
};

char *nargv[2] = {"/bin/login", NULL};

int determine_tty(int pid)
{
	int i;
	for (i = 0; i < NUM_TTYS; i++) {
		if (pid == tty_list[i].tty_pid) return i;
	}
	return -1;
}

int spawn_tty(int num)
{
	int pid, fd;

	if ((pid = fork()) != 0) {
#if USE_UTMP
		strcpy(entry.ut_line, tty_list[num].tty_name + 5);
		entry.ut_id[0] = tty_list[num].tty_name[8];
		entry.ut_id[1] = tty_list[num].tty_name[9];
		entry.ut_pid = pid;
		setutent();
		pututline(&entry);
#endif
		return pid;
	}
	setsid();

#if DEBUG
	close(0);
	close(1);
	close(2);
#endif

	if ((fd = open(tty_list[num].tty_name, O_RDWR)) < 0) {
		exit(1);
	}
	dup2(fd ,STDIN_FILENO);
	dup2(fd ,STDOUT_FILENO);
	dup2(fd ,STDERR_FILENO);

	execv(nargv[0], nargv);
#if DEBUG
	perror("execv");
#endif
	exit(1);
}

int main(int argc,char ** argv)
{
	int i;
	/* FIXME - Should really clean up utmp here */

#if DEBUG
	argv[0] = "init";
	int fd = open("/dev/tty1", 2);
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
#endif

#if USE_UTMP
	memset(&entry, 0, sizeof(struct utmp));
	entry.ut_type = INIT_PROCESS;
	time(&entry.ut_time);
	entry.ut_id[0] = 'l';
	entry.ut_id[1] = '1';
	entry.ut_pid = getpid();
	strcpy(entry.ut_user, "INIT");
	pututline(&entry);

	entry.ut_type = LOGIN_PROCESS;
	strcpy(entry.ut_user, "LOGIN");
#endif
	for (i = 0; i < NUM_TTYS; i++) {
		tty_list[i].tty_pid = spawn_tty(i);
	}
#if USE_UTMP
	endutent();
#endif

	while (1) {
		int pid = wait(NULL);
		if (-1 == pid)
			continue;
		i = determine_tty(pid);
#if USE_UTMP
		time(&entry.ut_time);
#endif
		tty_list[i].tty_pid = spawn_tty(i);
#if USE_UTMP
		endutent();
#endif
	}
}
