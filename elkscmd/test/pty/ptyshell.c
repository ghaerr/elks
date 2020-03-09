/*
 * ptyshell - demo program for ELKS pseudo tty support
 *
 * This program opens a master pseudo tty and then forks a child process.
 * This child process opens a slave tty and starts a shell in it.
 * The master process read user input from stdin (could be a network connection too)
 * and writes it to the master pseudo tty. This sends this to the slave tty which gets
 * input and output from the shell started by the child process. The output from the
 * shell process is written by the slave tty to the master tty and read from the master
 * process. Here, it will write that to stdout but it could be a network connection too.
 * Signal handling does not work yet. The input is line oriented.
 * If the message "cannot fork" appears, there is insufficient memory available. Disable
 * some drivers to free memory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>

#define MAX_INPUT 250

static int tfd, tfs;
pid_t pid;

int term_init();

int main(int argc, char ** argv)
{
  	int rc;
	char input[MAX_INPUT];

	printf("# "); /*simulate prompt */
	fflush(stdout);

	if (term_init() < 0) exit(1);

	while (1) {
	      rc = read(0, input, sizeof(input));

	      if (rc > 0)
              {
                  write(tfd, input, rc);
                  // Get the child's answer through the PTY
                  rc = read(tfd, input, MAX_INPUT-1);
                  if (rc > 0)
                  {
                    input[rc] = '\0'; // make NUL terminated
                    if (strcmp(input,"exitshell\n") == 0) {
		       fprintf(stderr, "Terminating processes and ptyshell\n");
		       kill(pid, SIGKILL);
		       return 0;
		    }
                    fprintf(stderr, "read from child:%s", input);
repeat:
		    /* read output from shell */
		    rc = read(tfd, input, sizeof(input));
		    input[rc] = '\0';
		    if (rc < 2) continue;

                    fprintf(stderr, "%s", input);
		    if (rc>0) goto repeat;
                  } else {
		    if (errno == EAGAIN){
		      fprintf(stderr, "errno == EAGAIN\n");
		      continue;
		    }
		    fprintf(stderr, "no input read\n");
		  }
	      }
	} //while
} //main

char * nargv[2] = {"/bin/sh", NULL};

void sigchild(int signo)
{
	fprintf(stderr, "Signal received:%d", signo);
	exit(0);
}

int term_init()
{
	char pty_name[12];
	int n = 0;
	int rc;

	struct termios slave_orig_term_settings; // Saved terminal settings
	struct termios new_term_settings; // Current terminal settings

again:
	sprintf(pty_name, "/dev/ptyp%d", n);
	if ((tfd = open(pty_name, O_RDWR | O_NONBLOCK)) < 0) {
		if ((errno == EBUSY) && (n < 3)) {
			n++;
			goto again;
		}
		fprintf(stderr, "Can't create pty %s\n", pty_name);
		return -1;
	}
	signal(SIGCHLD, sigchild);
	signal(SIGINT, sigchild);

	if ((pid = fork()) == -1) {
		fprintf(stderr, "No processes\n");
		return -1;
	}
	if (pid>0) {
		// Save the default parameters of the slave side of the PTY - unused yet
		rc = tcgetattr(tfs, &slave_orig_term_settings);
		new_term_settings = slave_orig_term_settings;
		// Set raw mode on the slave side of the PTY - not line oriented
		//cfmakeraw (&new_term_settings);
		//new_term_settings.c_lflag &= ~ECHO;
		tcsetattr (tfs, TCSANOW, &new_term_settings);

		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		close(tfd);

		setsid();
		pty_name[5] = 't'; /* results in: /dev/ttyp%d */
		if ((tfs = open(pty_name, O_RDWR)) < 0) {
			fprintf(stderr, "Child: Can't open pty %s\n", pty_name);
			exit(1);
		}

		dup2(tfs, STDIN_FILENO);
		dup2(tfs, STDOUT_FILENO);
		dup2(tfs, STDERR_FILENO);
		execv(nargv[0], nargv);
		perror("execv");
		exit(1);
	}
	return 0;
}
