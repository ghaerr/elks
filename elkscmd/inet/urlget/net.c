/* net.c Copyright 2000 by Michael Temari All Rights Reserved */
/* 04/05/2000 Michael Temari <Michael@TemWare.Com> */
/* 09/29/2001 Ported to ELKS(and linux) (Harry Kalogirou <harkal@rainbow.cs.unipi.gr>)  */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int net_connect(host, port)
char *host;
int port;
{
	int netfd;
	struct sockaddr_in in_adr;
	struct linger l;
	int ret;
	
	netfd = socket(AF_INET, SOCK_STREAM, 0);
	
	in_adr.sin_family = AF_INET;
	in_adr.sin_port = PORT_ANY;
	in_adr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(netfd, (struct sockaddr *)&in_adr, sizeof(struct sockaddr_in));
	if (ret < 0){
		perror("Bind failed");
		exit(1);
	}
	
	l.l_onoff = 1;	/* turn on linger option: will send RST on close*/
	l.l_linger = 0;	/* must be 0 to turn on option*/
	ret = setsockopt(netfd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
	if (ret < 0)
		perror("setsockopt RST");

	in_adr.sin_family = AF_INET;
	in_adr.sin_port = htons(port);
	in_adr.sin_addr.s_addr = in_gethostbyname(host);
	if (in_adr.sin_addr.s_addr == 0) {
		fprintf(stderr, "Can't find host '%s' in /etc/hosts\n", host);
		close (netfd);
		return -1;
	}

	ret = connect(netfd, (struct sockaddr *)&in_adr, sizeof(struct sockaddr_in));
	if (ret < 0){
		perror("Connect failed");
		close(netfd);
		return -1;
	}

	return(netfd);
}

/* if errflag set, send TCP RST on close, else send FIN */
void net_close(fd, errflag)
int fd;
int errflag;
{
	int ret;
	struct linger l;

	/* Send RST on close was previously turned on by net_connect*/
	if (!errflag) {
		l.l_onoff = 0;	/* turn off linger option: will send FIN on close*/
		l.l_linger = 0;
		ret = setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
		if (ret < 0)
			perror("setsockopt FIN");
	}
	close(fd);
}
