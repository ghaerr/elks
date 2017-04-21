#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#ifndef __linux__ 
#include <linuxmt/socket.h>
#include <linuxmt/un.h>
#include <linuxmt/in.h>
#include <linuxmt/arpa/inet.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include<string.h>
#endif

char *socket_path = "/var/uds";

static void usage()
{
	write(STDOUT_FILENO, "ELKS echoserver\n",16);
	write(STDOUT_FILENO, "  default  internet sockets using 127.0.0.1\n",44);
	write(STDOUT_FILENO, "  -u  use unix domain sockets\n",30);
	write(STDOUT_FILENO, "  -h  print this message\n\n",26);
}

int main(int argc, char *argv[]) {

  char buf[100];
  char	*cp;
  struct sockaddr_in addr_in;
#ifndef __linux__  
  struct sockaddr_un addr_un;    
#endif
  int fd,cl,rc,afunix,lv;
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
  lv=10;
  if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket error");
    exit(-1);
  }
  memset(&addr_in, 0, sizeof(addr_in));
  addr_in.sin_family = AF_INET;
  addr_in.sin_addr.s_addr = htons(INADDR_ANY);
  addr_in.sin_port = htons(2323);
  if (bind(fd, (struct sockaddr*)&addr_in, sizeof(addr_in)) == -1) {
    perror("bind error");
    exit(-1);
  } else {write(STDOUT_FILENO, "Echoserver:bind successful\n",27);}
} else { /* unix domain sockets */
#ifndef __linux__  
  lv=5;      
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
    unlink(socket_path);
  }
  if (bind(fd, (struct sockaddr*)&addr_un, sizeof(addr_un)) == -1) {
    perror("bind error");
    exit(-1);
  } else {write(STDOUT_FILENO, "Echoserver:bind successful\n",27);}
#endif
} /* afunix==0) */

  if (listen(fd, lv) == -1) {
    perror("listen error");
    close(fd);
    exit(-1);
  } 

  write(STDOUT_FILENO, "\nEcho server listening now\n",27); /* show prompt again */
  while (1) {
    if ( (cl = accept(fd, (struct sockaddr*)NULL, NULL)) == -1) {
      perror("accept error");
      continue;
    } else {write(STDOUT_FILENO, "echoserver:accept\n",18);}

    bzero( buf, sizeof(buf));
    while ( (rc=read(cl,buf,sizeof(buf))) > 0) {
      write(STDOUT_FILENO, "\rServer got:",12);
      write(STDOUT_FILENO, buf, rc+1);
      write(cl, buf, strlen(buf));
      bzero( buf, sizeof(buf));
    }
    if (rc == -1) {
      perror("read");
      close(cl);
      close(fd);
      exit(-1);
    } else if (rc == 0) {
      write(STDOUT_FILENO, "EOF\n",4);
      close(cl);
    }
  }

  close(fd);
  return 0;
}
