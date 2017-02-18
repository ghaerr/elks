#include <stdio.h>
#include <unistd.h>
#include <linuxmt/socket.h>
#include <linuxmt/un.h>
#include <stdlib.h>

char *socket_path = "/var/uds";

int main(int argc, char *argv[]) {
  struct sockaddr_un addr;
  char buf[100];
  int fd,cl,rc;

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
    unlink(socket_path);
  }

  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind error");
    exit(-1);
  }

  if (listen(fd, 5) == -1) {
    perror("listen error");
    close(fd);
    exit(-1);
  }

  printf("\nEcho server listening now\n"); /* show prompt again */
  while (1) {
    if ( (cl = accept(fd, NULL, NULL)) == -1) {
      perror("accept error");
      continue;
    }

    while ( (rc=read(cl,buf,sizeof(buf))) > 0) {
      printf("Server got:%.*s\n", rc+1, buf);
      write(cl, buf, 100);
      bzero( buf,100);
    }
    if (rc == -1) {
      perror("read");
      close(cl);
      close(fd);
      exit(-1);
    }
    else if (rc == 0) {
      printf("EOF\n");
      close(cl);
    }
  }

  close(fd);
  return 0;
}

