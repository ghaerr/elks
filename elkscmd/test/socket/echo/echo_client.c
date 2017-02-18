#include <sys/socket.h>
#include <linuxmt/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char *socket_path = "/var/uds";

int main(int argc, char *argv[]) {
  struct sockaddr_un addr;
  char buf[100];
  int fd,rc;

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
    bzero( buf, 100);
    if (rc=read(fd,buf,100) > 0)
    printf("Client got:%.*s\n", sizeof(buf), buf);
    bzero( buf,100);
  }
  close(fd);
  return 0;
}

