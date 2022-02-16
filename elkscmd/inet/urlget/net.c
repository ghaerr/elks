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

int net_connect(char *host, int port)
{
	int netfd, e;
	struct sockaddr_in in_adr;
	struct linger l;
	
	netfd = socket(AF_INET, SOCK_STREAM, 0);
	if (netfd < 0)
		return -1;
	
	in_adr.sin_family = AF_INET;
	in_adr.sin_port = PORT_ANY;
	in_adr.sin_addr.s_addr = INADDR_ANY;

	if (bind(netfd, (struct sockaddr *)&in_adr, sizeof(struct sockaddr_in)) < 0)
		goto error;
	
	l.l_onoff = 1;	/* turn on linger option: will send RST on close*/
	l.l_linger = 0;	/* must be 0 to turn on option*/
	setsockopt(netfd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));

	in_adr.sin_family = AF_INET;
	in_adr.sin_port = htons(port);
	in_adr.sin_addr.s_addr = in_gethostbyname(host);
	if (!in_adr.sin_addr.s_addr)
		goto error;

	if (in_connect(netfd, (struct sockaddr *)&in_adr, sizeof(struct sockaddr_in), 10) < 0)
		goto error;

	return netfd;
error:
	e = errno;
	close(netfd);
	errno = e;
	return -1;
}

/* if errflag set, send TCP RST on close, else send FIN */
void net_close(int fd, int errflag)
{
	/* Send RST on close was previously turned on by net_connect*/
	if (!errflag) {
		struct linger l;

		l.l_onoff = 0;	/* turn off linger option: will send FIN on close*/
		l.l_linger = 0;
		setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
	}
	close(fd);
}
