#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>

#ifndef __linux__ 
#include <linuxmt/socket.h>
#include <linuxmt/in.h>
#include <linuxmt/arpa/inet.h>
#else
#include <sys/types.h>
#include <netdb.h>
#endif
#include <time.h>

#define MAX_BUFFER 100
static char buf_in  [MAX_BUFFER];
static char buf_out [MAX_BUFFER];

static int tfd, tfs;
pid_t pid;
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
	if ((tfd = open(pty_name, O_RDWR)) < 0) {
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


static void usage()
{
	write(STDOUT_FILENO, "ELKS telnet server\n",16);
	write(STDOUT_FILENO, "  default  internet sockets using 127.0.0.1\n",44);
	write(STDOUT_FILENO, "  -h  print this message\n\n",26);
}

static int client_loop (int fdsock, int fdterm)
{
    int count_fd;
    fd_set fds_read;
    fd_set fds_write;

    int count;
    int count_in;
    int count_out;

    count_fd = (fdsock > fdterm) ? (fdsock + 1) : (fdterm + 1);

    count_in  = 0;
    count_out = 0;

    while (1) {
		FD_ZERO (&fds_read);
		if (!count_in)  FD_SET (fdsock, &fds_read);
		if (!count_out) FD_SET (fdterm, &fds_read);

		FD_ZERO (&fds_write);
		if (count_in)  FD_SET (fdterm, &fds_write);
		if (count_out) FD_SET (fdsock, &fds_write);

		count = select (count_fd, &fds_read, &fds_write, NULL, NULL);
		if (count < 0) {
			perror ("select");
			break;
		}

		if (!count_in && FD_ISSET (fdsock, &fds_read)) {
			count_in = read (fdsock, buf_in, MAX_BUFFER);
			if (count_in <= 0) {
				perror ("read sock");
				break;
			}
		}

		/* TODO: process IAC sequences */

		if (count_in && FD_ISSET (fdterm, &fds_write)) {
			count = write (fdterm, buf_in, count_in);
			if (count <= 0) {
				perror ("write term");
				break;
			}
			count_in -= count;
		}

		if (!count_out && FD_ISSET (fdterm, &fds_read)) {
			count_out = read (fdterm, buf_out, MAX_BUFFER);
			if (count_out <= 0) {
				perror ("read term");
				break;
			}
		}

		if (count_out && FD_ISSET (fdsock, &fds_write)) {
			count = write (fdsock, buf_out, count_out);
			if (count <= 0) {
				perror ("write sock");
				break;
			}
			count_out -= count;
		}
    }

    return 0;
}

/****************************************************************************************/
int main(int argc, char *argv[]) {

  char	*cp;
  struct sockaddr_in addr_in;
  int sockfd,connectionfd,ret,lv = 10;
  
#ifndef __linux__  
  if (argv[1][0] == '-') {
		cp = &argv[1][1];
		if (strcmp(cp, "h") == 0) {
			usage();
			exit (0);
		} else {
			usage();
			exit (0);			
		}
		argc--;
		argv++;
	} else if (argc == 2) {
			usage();
			exit (0);
	}
	  
#endif

  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket error");
    exit(-1);
  }
  memset(&addr_in, 0, sizeof(addr_in));
  addr_in.sin_family = AF_INET;
  addr_in.sin_addr.s_addr = htons(INADDR_ANY);
  addr_in.sin_port = htons(23);
  if (bind(sockfd, (struct sockaddr*)&addr_in, sizeof(addr_in)) == -1) {
    perror("bind error");
	close(sockfd);
    exit(-1);
  }

  if (listen(sockfd, lv) == -1) {
    perror("listen error");
    close(sockfd);
    exit(-1);
  } 

	if (fork())
		exit(0);
	ret = open("/dev/null", O_RDWR);
	dup2(ret, 0);
	dup2(ret, 1);
	dup2(ret, 2);
	if (ret > 2)
		close(ret);

	while (1) {
		connectionfd = accept (sockfd, (struct sockaddr *) NULL, NULL);
		if (connectionfd < 0) {
			perror ("accept");
			break;
		}

		term_init ();

		client_loop (connectionfd, tfd);

		kill (pid, SIGKILL);
		close (connectionfd);
		close (tfd);
	}

	close (sockfd);
	return 0;
}
