/*
 * telnetd for ELKS
 *
 * Debugged and added IAC processing.
 * Greg Haerr May 2020
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "telnet.h"

//#define RAWTELNET	/* set in telnet and telnetd for raw telnet without IAC*/

#define MAX_BUFFER 512		/* should be equal to TDB_WRITE_MAX and PTYOUTQ_SIZE*/
static char buf_in  [MAX_BUFFER];
static char buf_out [MAX_BUFFER];

char *nargv[2] = {"/bin/login", NULL};

static pid_t term_init(int *pty_fd)
{
	int n = 0;
	int tty_fd;
	pid_t pid;
	char pty_name[12];

again:
	sprintf(pty_name, "/dev/ptyp%d", n);
	if ((*pty_fd = open(pty_name, O_RDWR)) < 0) {
		if ((errno == EBUSY) && (n < 3)) {
			n++;
			goto again;
		}
		fprintf(stderr, "Can't create pty %s\n", pty_name);
		return -1;
	}
	signal(SIGCHLD, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	
	if ((pid = fork()) == -1) {
		fprintf(stderr, "telnetd: No processes\n");
		return -1;
	}
	if (!pid) {
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		close(*pty_fd);

		setsid();
		pty_name[5] = 't'; /* results in: /dev/ttyp%d */
		if ((tty_fd = open(pty_name, O_RDWR)) < 0) {
			fprintf(stderr, "telnetd: Can't open pty %s\n", pty_name);
			exit(1);
		}

		/* set login tty to sane mode, since getty not called*/
{
		struct termios termios;
		tcgetattr(tty_fd, &termios);

		termios.c_lflag |= ISIG | ICANON | ECHO | ECHOE;
		termios.c_lflag &= ~(IEXTEN | ECHOK | NOFLSH | ECHONL);
		termios.c_iflag |= BRKINT | ICRNL;
		termios.c_iflag &= ~(IGNBRK | IGNPAR | PARMRK | INPCK | ISTRIP | INLCR | IGNCR | IXON | IXOFF | IXANY);
		termios.c_oflag |= OPOST | ONLCR;
		termios.c_oflag &= ~XTABS;
		termios.c_cflag &= ~PARENB;
		termios.c_cflag |= CS8 | CREAD | HUPCL;
		termios.c_cflag |= CLOCAL;			/* ignore modem control lines*/
		termios.c_cc[VMIN] = 1;
		termios.c_cc[VTIME] = 0;

		/* turn off echo - not needed with telnet ECHO option on*/
		//termios.c_lflag &= ~ECHO;
		tcsetattr(tty_fd, TCSANOW, &termios);
}

		dup2(tty_fd, STDIN_FILENO);
		dup2(tty_fd, STDOUT_FILENO);
		dup2(tty_fd, STDERR_FILENO);
		execv(nargv[0], nargv);
		perror("execv");
		exit(1);
	}
	return pid;
}

static void telnet_init(int ofd)
{
#ifndef RAWTELNET
  tel_init();

  telopt(ofd, WILL, TELOPT_SGA);
  telopt(ofd, DO,   TELOPT_SGA);
  telopt(ofd, WILL, TELOPT_BINARY);
  telopt(ofd, DO,   TELOPT_BINARY);
  telopt(ofd, WILL, TELOPT_ECHO);
  //telopt(ofd, DO,   TELOPT_WINCH);
#endif
}

static void client_loop (int fdsock, int fdterm)
{
    fd_set fds_read;
    fd_set fds_write;
    int count;
    int count_in = 0;
    int count_out = 0;
    int count_fd = (fdsock > fdterm) ? (fdsock + 1) : (fdterm + 1);

	telnet_init(fdsock);

    while (1) {
		FD_ZERO (&fds_read);
		if (!count_in)  FD_SET (fdsock, &fds_read);
		if (!count_out) FD_SET (fdterm, &fds_read);

		FD_ZERO (&fds_write);
		if (count_in)  FD_SET (fdterm, &fds_write);
		if (count_out) FD_SET (fdsock, &fds_write);

		count = select (count_fd, &fds_read, &fds_write, NULL, NULL);
		if (count < 0) {
			perror ("telnetd select");
			break;
		}

		/* network -> login process*/
		if (!count_in && FD_ISSET (fdsock, &fds_read)) {
			count_in = read (fdsock, buf_in, MAX_BUFFER);
			if (count_in <= 0) {
				if (count < 0)
					perror ("telnetd read sock");
				break;
			}
		}
		if (count_in && FD_ISSET (fdterm, &fds_write)) {
#ifdef RAWTELNET
			count = write (fdterm, buf_in, count_in);
#else
			tel_in(fdterm, fdsock, buf_in, count_in);
#endif
			count_in = 0;
		}

		/* login process -> network*/
		if (!count_out && FD_ISSET (fdterm, &fds_read)) {
			count_out = read (fdterm, buf_out, MAX_BUFFER);
			if (count_out <= 0) {
				if (count_out < 0)
					perror ("telnetd read term");
				break;
			}
		}
		if (count_out && FD_ISSET (fdsock, &fds_write)) {
#ifdef RAWTELNET
			count = write (fdsock, buf_out, count_out);
#else
			tel_out(fdsock, buf_out, count_out);
#endif
			count_out = 0;
		}
    }
}

#if 0
void sigchild(void)
{
	waitpid(-1, NULL, WNOHANG);
	signal(SIGCHLD, sigchild);
}
#endif

int main(int argc, char **argv)
{
	struct sockaddr_in addr_in;
	int sockfd,connectionfd,fd;
	int pty_fd, ret;
	pid_t pid;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "telnetd: Can't open socket (check if ktcp is running)\n");
		exit(-1);
	}

	/* set local port reuse, allows server to be restarted in less than 10 secs */
	ret = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(int)) < 0)
		perror("SO_REUSEADDR");

	/* set small listen buffer to save ktcp memory */
	ret = SO_LISTEN_BUFSIZ;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &ret, sizeof(int)) < 0)
		perror("SO_RCVBUF");

	memset(&addr_in, 0, sizeof(addr_in));
	addr_in.sin_family = AF_INET;
	addr_in.sin_addr.s_addr = htons(INADDR_ANY);
	addr_in.sin_port = htons(23);
	if (bind(sockfd, (struct sockaddr *)&addr_in, sizeof(addr_in)) == -1) {
		fprintf(stderr, "telnetd: bind error (may already be running)\n");
		close(sockfd);
		exit(-1);
	}

	if (listen(sockfd, 3) == -1) {
		perror("listen error");
		close(sockfd);
		exit(-1);
	}

	/* become daemon, debug output on 1 and 2*/
	if ((ret = fork()) == -1) {
		fprintf(stderr, "telnetd: Can't fork to become daemon\n");
		exit(1);
	}
	if (ret) exit(0);
	fd = open("/dev/console", O_RDWR);
	close(STDIN_FILENO);
	dup2(fd, STDOUT_FILENO);
	dup2(fd, STDERR_FILENO);
	if (fd > STDERR_FILENO)
		close(fd);
	setsid();
	//signal(SIGCHLD, sigchild);

	while (1) {
		connectionfd = accept (sockfd, (struct sockaddr *) NULL, NULL);
		if (connectionfd < 0) {
			perror ("telnetd accept");
			break;
		}

		waitpid(-1, NULL, WNOHANG);		/* reap previous accepts*/

		if ((ret = fork()) == -1)		/* handle new accept*/
			fprintf(stderr, "telnetd: No processes\n");
		else if (ret == 0) {
			close(sockfd);
			pid = term_init(&pty_fd);
			if (pid != -1) {
				client_loop (connectionfd, pty_fd);

				kill (pid, SIGKILL);
				waitpid(pid, NULL, 0);
			}
			close (connectionfd);
			close (pty_fd);
			exit(0);
		} else {
			close(connectionfd);
		}
	}

	close (sockfd);
	return 0;
}
