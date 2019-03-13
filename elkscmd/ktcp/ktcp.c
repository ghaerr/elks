/*
 * This file is part of the ELKS TCP/IP stack
 *
 * (C) 2001 Harry Kalogirou (harkal@rainbow.cs.unipi.gr)
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#include "slip.h"
#include "tcpdev.h"
#include "timer.h"
#include <linuxmt/arpa/inet.h>
#include "ip.h"
#include "tcp.h"
#include "netconf.h"
#include "deveth.h"

#ifdef DEBUG
#define debug	printf
#else
#define debug(s)
#endif

extern int tcp_timeruse;

//static int tcpdevfd; /* defined in ip.h */
static int intfd;

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

void ktcp_run(void)
{
    fd_set fdset;
    struct timeval timeint, *tv;
    int count;

    while (1) {
	if (tcp_timeruse > 0 || tcpcb_need_push > 0) {
	    timeint.tv_sec  = 1;
	    timeint.tv_usec = 0;
	    tv = &timeint;
	} else tv = NULL;

	FD_ZERO(&fdset);
	FD_SET(intfd, &fdset);
	FD_SET(tcpdevfd, &fdset);
	count = select(intfd > tcpdevfd ? intfd + 1 : tcpdevfd + 1, &fdset, NULL, NULL, tv);
	if (count < 0) return;

	Now = timer_get_time();

	tcp_update();

	if (FD_ISSET(intfd, &fdset)) {
		if (dev->type == 0) slip_process();
		else deveth_process();
	}
	if (FD_ISSET(tcpdevfd, &fdset)) tcpdev_process();

	if (tcp_timeruse > 0) tcp_retrans();

#ifdef DEBUG
	tcpcb_printall();
#endif
    }
}

int main(int argc,char **argv)
{
    __u8 * addr;

    const char dname[9];
    sprintf(dname, "/dev/eth");

    if (argc < 3) {
	printf("Syntax :\n    %s local_ip [interface] [gateway] [netmask]\n", argv[0]);
	exit(3);
    }

    debug("KTCP: 1. local_ip \n");
    local_ip = in_aton(argv[1]);

    if (argc > 3) gateway_ip = in_aton(argv[3]);
    else gateway_ip = in_aton("10.0.2.2"); /* use dhcp when implemented */

    if (argc > 4) netmask_ip = in_aton(argv[4]);
    else netmask_ip = in_aton("255.255.255.0");

    /*
    addr = (__u8 *) &local_ip;
    printf ("local_ip: %2X.%2X.%2X.%2X\n", addr [0], addr [1], addr [2], addr[3]);
    addr = (__u8 *) &gateway_ip;
    printf ("gateway_ip: %2X.%2X.%2X.%2X\n", addr [0], addr [1], addr [2], addr [3]);
    addr = (__u8 *) &netmask_ip;
    printf ("netmask_ip: %2X.%2X.%2X.%2X\n", addr [0], addr [1], addr [2], addr [3]);
    */

    debug("\nKTCP: 2. init tcpdev\n");
    if ((tcpdevfd = tcpdev_init("/dev/tcpdev")) < 0) exit(1);

    debug("KTCP: 3. init interface\n");
    if (strcmp(argv[2],dname) == 0) {
	debug("Init /dev/eth\n");
    	dev->type = 1;
	intfd = deveth_init(dname);
	if (intfd < 0) {
	      printf("failed to open /dev/eth [%d]\n", intfd);
	      exit(1);
	}
    } else { /* fall back to slip */
	dev->type=0;
	intfd = slip_init(argv[2]);
	if (intfd < 0) exit(2);
    }

    arp_init ();

    debug("KTCP: 4. ip_init()\n");
    ip_init();

    debug("KTCP: 5. icmp_init()\n");
    icmp_init();

    debug("KTCP: 6. tcp_init()\n");
    tcp_init();

    debug("KTCP: 7. netconf_init()\n");
    netconf_init();

    debug("KTCP: 8. ktcp_run()\n");
    ktcp_run();

    debug("KTCP: 9. exit(0)\n");
    exit(0);
}
