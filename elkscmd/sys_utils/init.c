#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <utmp.h>
#include <fcntl.h>
#include <string.h>

struct exec_struct {
	char tty_name[12];
	pid_t tty_pid;	
};

#define NUM_TTYS 3

struct exec_struct tty_list[] = {
	{"/dev/tty1", 0},
	{"/dev/tty2", 0},
	{"/dev/tty3", 0}
};

char * nargv[2] = {"/bin/getty", NULL};

int determine_tty(int pid)
{
	int i;
	for (i = 0; i < NUM_TTYS; i++) {
		if (pid == tty_list[i].tty_pid) return i;
	}
	return -1;
}

struct utmp entry;

int spawn_tty(int num)
{
	int pid, fd;

	if ((pid = fork()) != 0) {
		strcpy(entry.ut_line, tty_list[num].tty_name + 5);
		entry.ut_id[0] = tty_list[num].tty_name[8];
		entry.ut_id[1] = tty_list[num].tty_name[9];
		entry.ut_pid = pid;
		setutent();
		pututline(&entry);
		return pid;
	}
	if ((fd = open(tty_list[num].tty_name, O_RDWR)) < 0) {
		exit(1);
	}
	dup2(fd ,STDIN_FILENO);
	dup2(fd ,STDOUT_FILENO);
	dup2(fd ,STDERR_FILENO);

	execv(nargv[0], nargv);
	exit(1);
}

void main(int argc,char ** argv)
{
	int i, pid, fd;
	/* FIXME - Should really clean up utmp here */

	argv[0] = "init";

/*	fd = open("/dev/tty1", 2);
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2); */

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
	for (i = 0; i < NUM_TTYS; i++) {
		pid = spawn_tty(i);
		tty_list[i].tty_pid = pid;
	}
	endutent();

/*	printf("PID of init is %d\n",getpid());
	fflush(stdout); */

	while(1) {
		pid = wait(NULL);
		i = determine_tty(pid);
		tty_list[i].tty_pid = 0;
		time(&entry.ut_time);
		tty_list[i].tty_pid = spawn_tty(i);
		endutent();
	}
}
