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
#include "netorder.h"
#include "ip.h"
#include "tcp.h"
#include "netconf.h"

#ifdef DEBUG
#define debug	printf
#else
#define debug(s)
#endif

extern int tcp_timeruse;

static int sfd, tcpdevfd;

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

    while (1) {
	if (tcp_timeruse > 0 || tcpcb_need_push > 0) {
	    timeint.tv_sec  = 1;
	    timeint.tv_usec = 0;
	    tv = &timeint;
	} else
	    tv = NULL;

	FD_ZERO(&fdset);
	FD_SET(sfd, &fdset);
	FD_SET(tcpdevfd, &fdset);
	select(sfd > tcpdevfd ? sfd + 1 : tcpdevfd + 1,
	       &fdset, NULL, NULL, tv);

	Now = timer_get_time();

	tcp_update();

	if (FD_ISSET(sfd, &fdset))
	    slip_process();

	if (FD_ISSET(tcpdevfd, &fdset))
	    tcpdev_process();

	if (tcp_timeruse > 0)
	    tcp_retrans();

#ifdef DEBUG
	tcpcb_printall();
#endif

    }
}

int main(int argc,char **argv)
{
    if (argc < 3) {
	printf("Syntax :\n    %s local_ip slip_tty\n",argv[0]);
	exit(3);
    }

    debug("KTCP: Mark 1.\n");
    local_ip = in_aton(argv[1]);

    debug("KTCP: Mark 2.\n");
    if ((tcpdevfd = tcpdev_init("/dev/tcpdev")) < 0)
	exit(1);

    debug("KTCP: Mark 3.\n");
    if ((sfd = slip_init(argv[2])) < 0)
	exit(2);

    debug("KTCP: Mark 4.\n");
    ip_init();

    debug("KTCP: Mark 5.\n");
    icmp_init();

    debug("KTCP: Mark 6.\n");
    tcp_init();

    debug("KTCP: Mark 7.\n");
    netconf_init();

    debug("KTCP: Mark 8.\n");
    ktcp_run();

    debug("KTCP: Mark 9.\n");
    exit(0);
}
