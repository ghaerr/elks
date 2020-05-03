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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "slip.h"
#include "tcp.h"
#include "tcpdev.h"
#include "tcp_output.h"
#include "timer.h"
#include <linuxmt/arpa/inet.h>
#include "ip.h"
#include "icmp.h"
#include "netconf.h"
#include "deveth.h"
#include "arp.h"

#ifdef DEBUG
#define debug	printf
#else
#define debug(s)
#endif

char deveth[] = "/dev/eth";

extern int tcp_timeruse;

static int tcpdevfd;
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
    //__u8 * addr;
    int daemon = 0;
    speed_t baudrate = 0;
    char *progname = argv[0];

    if (argc > 1 && !strcmp("-b", argv[1])) {
	daemon = 1;
	argc--;
	argv++;
    }
    if (argc > 1 && !strcmp("-s", argv[1])) {
	if (argc <= 2) goto usage;
	baudrate = atol(argv[2]);
	argc -= 2;
	argv += 2;
    }
    if (argc < 3) {
usage:
	printf("Usage: %s [-b] [-s baud] local_ip [interface] [gateway] [netmask]\n", progname);
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

    /* must exit() in next two stages on error to reset kernel tcpdev_inuse to 0*/
    debug("\nKTCP: 2. init tcpdev\n");
    if ((tcpdevfd = tcpdev_init("/dev/tcpdev")) < 0) exit(1);

    debug("KTCP: 3. init interface\n");
    if (strcmp(argv[2],deveth) == 0) {
	debug("Init /dev/eth\n");
    	dev->type = 1;
	intfd = deveth_init(deveth);
	if (intfd < 0) exit(1);
    } else { /* fall back to slip */
	dev->type=0;
	intfd = slip_init(argv[2], baudrate);
	if (intfd < 0) exit(2);
    }

    /* become daemon now that tcpdev_inuse race condition over*/
    if (daemon) {
	if (fork())
	    exit(0);
	close(0);
	close(1);
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
