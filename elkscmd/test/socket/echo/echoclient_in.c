#include <linuxmt/socket.h>
#include <linuxmt/un.h>
#include <linuxmt/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linuxmt/arpa/inet.h>

#define INTERNET

char *socket_path = "/var/uds";

unsigned long int in_aton(const char *str)
{
    unsigned long l = 0;
    unsigned int val;
    int i;

    for (i = 0; i < 4; i++) {
	l <<= 8;
	if (*str != '\0') {
	    val = 0;
	    while (*str != '\0' && *str != '.') {
		val *= 10;
		val += *str++ - '0';
	    }
	    l |= val;
	    if (*str != '\0')
		str++;
	}
    }
    return htonl(l);
}

int main(int argc, char *argv[]) {
  char buf[100];
  int fd,rc;

#ifdef INTERNET
  struct sockaddr_in addr;  
  if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket error");
    exit(-1);
  }
  memset(&addr, 0, sizeof(addr));

addr.sin_family = AF_INET;
addr.sin_port = 0; /* any port */
addr.sin_addr.s_addr = INADDR_ANY;

if (bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in))==-1) {
        	perror("Bind failed");
		exit(1);
}
  
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = in_aton("10.0.2.2");
  addr.sin_port = htons(2323);
#else
  struct sockaddr_un addr;
  if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("socket error");
    exit(-1);
  }
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX; 
  if (*socket_path == '\0') {
    *addr.sun_path = '\0';
    strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2);
  } else {
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
  }
#endif
  
  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("connect error");
    close(fd);
    exit(-1);
  }
  printf("\nType any string and press enter to send it to the server.\n");
  printf("ESC and enter to quit.\n");
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
    if (rc=read(fd,buf,100) > 0)
    printf("Server returned:%s\n",buf);  
    bzero( buf, sizeof(buf));
  }
  close(fd);
  return 0;
}

