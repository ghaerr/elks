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
#ifndef __LINUX__
#include <linuxmt/in.h>
#include <linuxmt/net.h>
#include <linuxmt/time.h>
#include "../httpd/netorder.h"
#else
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

unsigned long int in_aton(str)
const char *str;
{
        unsigned long l;
        unsigned int val;
        int i;

        l = 0;
        for (i = 0; i < 4; i++)
        {
                l <<= 8;
                if (*str != '\0')
                {
                        val = 0;
                        while (*str != '\0' && *str != '.')
                        {
                                val *= 10;
                                val += *str - '0';
                                str++;
                        }
                        l |= val;
                        if (*str != '\0')
                                str++;
                }
        }
        return(htonl(l));   
}

int net_connect(host, port)
char *host;
int port;
{
	int netfd;
	struct sockaddr_in in_adr;
	int ret;
	
	netfd = socket(AF_INET, SOCK_STREAM, 0);
	
	in_adr.sin_family = AF_INET;
	in_adr.sin_port = 0; /* any port */
	in_adr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(netfd, (struct sockaddr *)&in_adr, sizeof(struct sockaddr_in));
	if (ret < 0){
		perror("Bind failed");
		exit(1);
	}
	
	in_adr.sin_family = AF_INET;
	in_adr.sin_port = htons(port);
	in_adr.sin_addr.s_addr = in_aton(host);

	ret = connect(netfd, (struct sockaddr *)&in_adr, sizeof(struct sockaddr_in));
	if (ret < 0){
		perror("Connection failed");
		exit(1);
	}

	return(netfd);
}
