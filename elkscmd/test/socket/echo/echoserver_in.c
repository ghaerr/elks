#include <stdio.h>
#include <unistd.h>
#include <linuxmt/socket.h>
#include <linuxmt/un.h>
#include <linuxmt/in.h>
#include <stdlib.h>
#include <linuxmt/arpa/inet.h>

#define INTERNET

char *socket_path = "/var/uds";

int main(int argc, char *argv[]) {

  char buf[100];
  int fd,cl,rc;

#ifdef INTERNET
  struct sockaddr_in addr;  
  if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket error");
    exit(-1);
  }
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htons(INADDR_ANY);
  addr.sin_port = htons(23);
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
    unlink(socket_path);
  }
#endif
  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind error");
    exit(-1);
  } else {printf("echoserver:bind\n");}
#ifdef INTERNET
  if (listen(fd, 10) == -1) {
#else
  if (listen(fd, 5) == -1) {
#endif    
    perror("listen error");
    close(fd);
    exit(-1);
  } else {printf("echoserver:listen\n");}

  printf("\nEcho server listening now\n"); /* show prompt again */
  while (1) {
    if ( (cl = accept(fd, (struct sockaddr*)NULL, NULL)) == -1) {
      perror("accept error");
      continue;
    } else {printf("echoserver:accept\n");}

    while ( (rc=read(cl,buf,sizeof(buf))) > 0) {
      //printf("ES\n");
      printf("Server got:%.*s\n", rc+1, buf);
       //printf("ES:%2X%2X %2X%2X %2X%2X %2X%2X\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7]);
      write(cl, buf, strlen(buf));
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

#if 0
 
    char str[100];
    int listen_fd, comm_fd, rc;
 
    struct sockaddr_in servaddr;
 
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
 
    bzero( &servaddr, sizeof(servaddr));
 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(23);
 
    if (bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == 0)
      printf("echoserver:bind\n");
 
    if (listen(listen_fd, 10)==0) printf("echoserver:listen\n");
 
    if ((comm_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL))== 0) 
      printf("echoserver:accept\n");
 
    while(1)
    {
 
        bzero( str, 100);
 
        if (rc=read(comm_fd,str,100)>0){
	  
        //printf("Echoing back - %s",str);
	//printf("Server: got %u chars: %.*s\n", rc-1, rc, buf);
	  printf("Server: got %u chars\n", rc-1);
 
        write(comm_fd, str, strlen(str)+1);
	}
 
    }
#endif