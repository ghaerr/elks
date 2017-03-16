#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#ifndef __linux__ 
#include <linuxmt/socket.h>
#include <linuxmt/in.h>
#include <linuxmt/arpa/inet.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include<string.h>
#endif

#define MAX_BUFFER 100

static int tfd, tfs;
pid_t pid;
char * nargv[2] = {"/bin/sh", NULL};

void tel_out(int fdout, char *buf, int size)
{
  write(fdout, buf, size);
}

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


static void usage()
{
	write(STDOUT_FILENO, "ELKS telnet server\n",16);
	write(STDOUT_FILENO, "  default  internet sockets using 127.0.0.1\n",44);
	write(STDOUT_FILENO, "  -h  print this message\n\n",26);
}


/****************************************************************************************/
int main(int argc, char *argv[]) {

  char buffer[MAX_BUFFER];
  char	*cp;
  struct sockaddr_in addr_in;
  int sockfd,connectionfd,rc,afunix,lv;
  afunix=0;
  
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

  lv=10;
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
    exit(-1);
  } //else {write(STDOUT_FILENO, "Telnetserver:bind successful\n",29);}

  if (listen(sockfd, lv) == -1) {
    perror("listen error");
    close(sockfd);
    exit(-1);
  } 
   
  term_init();
  
  write(STDOUT_FILENO, "Telnet server listening now\n",28); /* show prompt again */
  while (1) {
    if ( (connectionfd = accept(sockfd, (struct sockaddr*)NULL, NULL)) == -1) {
      perror("accept error");
      continue;
    } else {write(STDOUT_FILENO, "Telnetserver:accept connection\n",31);}
    
/* login process **************************************************************************/    
    while (1) {
      tel_out(connectionfd, (char *)"Welcome to the ELKS telnet server\n\nlogin: ", 42);
read_again:
      if (rc=read(connectionfd,buffer,sizeof(buffer)) > 0) 
	if (strcmp(buffer,"root\r\n")==0) {
	  goto login_successful;
	} else {
	  continue;
	} 
      else {
	  goto read_again;
      }
    }
login_successful:
    sprintf(buffer,"# "); /*simulate prompt */
    tel_out(connectionfd, (char *)buffer, strlen(buffer));

/* remote terminal session *****************************************************************/    
    fprintf(stderr, "Commands entered by remote terminal:\n"); /*will work ok only with this log*/
    while ( (rc=read(connectionfd,buffer,sizeof(buffer))) > 0) {
		  //printf("%X,%X,%X,%X\n",buffer[0],buffer[1],buffer[2],buffer[3]); /*test for IAC*/
		  write(tfd, buffer, rc);
                  // Get the child's answer through the PTY
                  rc = read(tfd, buffer, MAX_BUFFER-1); 
                  if (rc > 0)
                  {
                    buffer[rc] = '\0'; // make NUL terminated
                    if (strcmp(buffer,"exitelks\n")==0) {
		       fprintf(stderr, "Terminating processes and telnet server\n");
		       close(connectionfd);
		       close(sockfd);
		       kill(pid, SIGKILL);
		       return 0; 
		    }
                    fprintf(stderr, ">%s", buffer);
repeat:		  	
		    /* read output from shell */
		    rc = read(tfd, buffer, sizeof(buffer));
		    buffer[rc] = '\0';
		    if (rc < 2) continue;
		    tel_out(connectionfd, (char *)buffer, strlen(buffer));
		    if (rc>0) goto repeat;
                  } else {
		    if (errno == EAGAIN){
		      fprintf(stderr, "errno == EAGAIN\n");
		      continue;
		    }
		    fprintf(stderr, "no input read\n");
		  }      
    }
    
    if (rc == -1) {
      perror("read");
      close(connectionfd);
      close(sockfd);
      exit(-1);
    }
  }
  close(sockfd);
  return 0;
}
