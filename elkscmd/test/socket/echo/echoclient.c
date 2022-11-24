#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

char *socket_path = "/var/uds";

static void usage()
{
	write(STDOUT_FILENO, "ELKS echoclient\n",16);
	write(STDOUT_FILENO, "  default  internet sockets using 127.0.0.1\n",44);
	write(STDOUT_FILENO, "  -u  use unix domain sockets\n",30);
	write(STDOUT_FILENO, "  -h  print this message\n\n",26);
}

int main(int argc, char *argv[]) {
  char buf[100];
  char	*cp;
  int fd,rc,afunix;
  struct sockaddr_in addr_in;
#ifndef __linux__   
  struct sockaddr_un addr_un;
#endif
  afunix=0;
  
#ifndef __linux__  
  if (argv[1][0] == '-') {
		cp = &argv[1][1];
		if (strcmp(cp, "u") == 0) {
			afunix = 1;
		}
		else if (strcmp(cp, "h") == 0) {
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

if ( afunix == 0 ) { /* Internet */
  if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket error");
    exit(-1);
  }
  memset(&addr_in, 0, sizeof(addr_in));

addr_in.sin_family = AF_INET;
addr_in.sin_port = 0; /* any port */
addr_in.sin_addr.s_addr = INADDR_ANY;

if (bind(fd, (struct sockaddr *)&addr_in, sizeof(struct sockaddr_in))==-1) {
        	perror("Bind failed");
		exit(1);
}
  
  addr_in.sin_family = AF_INET;
#ifndef __linux__   
  addr_in.sin_addr.s_addr = in_aton("10.0.2.2");
#else  
  inet_pton(AF_INET,"127.0.0.1",&(addr_in.sin_addr));
#endif  
  addr_in.sin_port = htons(2323);

  if (connect(fd, (struct sockaddr*)&addr_in, sizeof(addr_in)) == -1) {
    perror("connect error");
    close(fd);
    exit(-1);
  }

} else { /* unix domain sockets */
#ifndef __linux__   
  if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("socket error");
    exit(-1);
  }
  memset(&addr_un, 0, sizeof(addr_un));
  addr_un.sun_family = AF_UNIX; 
  if (*socket_path == '\0') {
    *addr_un.sun_path = '\0';
    strncpy(addr_un.sun_path+1, socket_path+1, sizeof(addr_un.sun_path)-2);
  } else {
    strncpy(addr_un.sun_path, socket_path, sizeof(addr_un.sun_path)-1);
  }
  if (connect(fd, (struct sockaddr*)&addr_un, sizeof(addr_un)) == -1) {
    perror("connect error");
    close(fd);
    exit(-1);
  }
#endif  
} /* afunix==0) */
  
  write(STDOUT_FILENO, "\nType any string and press enter to send it to the server.\n",59);
  write(STDOUT_FILENO, "ESC and enter to quit.\n",23);
  while( (rc=read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
    if (buf[0]==27) exit (0);
    if (write(fd, buf, rc) != rc) {
      if (rc > 0) fprintf(stderr,"partial write");
      else {
        perror("write error");
	close(fd);
        exit(-1);
      }
    }
    bzero( buf, sizeof(buf));  
    if ((rc=read(fd,buf,100) > 0))
      write(STDOUT_FILENO, "Server returned:",17);
    write(STDOUT_FILENO, buf, sizeof(buf));  
    write(STDOUT_FILENO, "\r", 1);
    bzero( buf, sizeof(buf));
  }
  close(fd);
  return 0;
}
