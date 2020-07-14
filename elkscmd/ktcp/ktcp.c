/*
 * This file is part of the ELKS TCP/IP stack
 *
 * (C) 2001 Harry Kalogirou (harkal@rainbow.cs.unipi.gr)
 *
 * Major debugging by Greg Haerr May 2020
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
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>

#include "slip.h"
#include "tcp.h"
#include "tcp_output.h"
#include "tcp_cb.h"
#include "tcpdev.h"
#include "timer.h"
#include <linuxmt/arpa/inet.h>
#include "ip.h"
#include "icmp.h"
#include "netconf.h"
#include "deveth.h"
#include "arp.h"

ipaddr_t local_ip;
ipaddr_t gateway_ip;
ipaddr_t netmask_ip;

char foo[] = "/dev/eth";	// FIXME remove after memory errors

#define DEFAULT_IP		"10.0.2.15"
#define DEFAULT_GATEWAY		"10.0.2.2"
#define DEFAULT_NETMASK		"255.255.255.0"

/* defaults*/
int linkprotocol = 	LINK_ETHER;
unsigned int MTU =	SLIP_MTU;
char ethdev[] = 	"/dev/eth";
char *serdev = 		"/dev/ttyS0";
speed_t baudrate = 	57600;

int dflag;
static int intfd;	/* interface fd*/

void setp(unsigned char **p)
{
	*p = 0;
}

char *zero;
void printp()
{
unsigned char *p;
//int i;
setp(&p);

//printf(zero="ZERO\n");
//for (i=0; i<16; i++) printf("%d (%c) ", p[i], p[i]);
//printf("\n");
}

void ktcp_run(void)
{
    fd_set fdset;
    struct timeval timeint, *tv;
    int count;
extern int tcp_timeruse;
extern int cbs_in_time_wait;
extern int cbs_in_user_timeout;

    while (1) {
	//if (tcp_timeruse > 0 || tcpcb_need_push > 0) {
	if (tcp_timeruse > 0 || tcpcb_need_push > 0 || cbs_in_time_wait > 0 || cbs_in_user_timeout > 0) {

	    timeint.tv_sec  = 1;
	    timeint.tv_usec = 0;
	    tv = &timeint;
	} else tv = NULL;

printp();

	FD_ZERO(&fdset);
	FD_SET(intfd, &fdset);
	FD_SET(tcpdevfd, &fdset);
	count = select(intfd > tcpdevfd ? intfd + 1 : tcpdevfd + 1, &fdset, NULL, NULL, tv);
	if (count < 0)
		return;	//FIXME

	Now = timer_get_time();

	/* expire timeouts and push data*/
	tcp_update();

	/* process received packets*/
	if (FD_ISSET(intfd, &fdset)) {
		if (linkprotocol == LINK_ETHER)
			eth_process();
		else slip_process();
	}

	/* process application socket actions*/
	if (FD_ISSET(tcpdevfd, &fdset))
		tcpdev_process();

	/* check for retransmit needed*/
	if (tcp_timeruse > 0)
		tcp_retrans();

	tcpcb_printall();
    }
}

void catch(int sig)
{
    printf("ktcp: exit signal %d\n", sig);
}

static void usage(void)
{
    printf("Usage: ktcp [-b] [-d] [-m MTU] [-p eth|slip|cslip] [-s baud] [-l device] [local_ip] [gateway] [netmask]\n");
    exit(1);
}

int main(int argc,char **argv)
{
    int ch;
    int bflag = 0;
    static char *linknames[3] = { "ethernet", "slip", "cslip" };
printp();
//printf("ZERO is at %x\n", zero);

    while ((ch = getopt(argc, argv, "bdm:p:s:l:")) != -1) {
	switch (ch) {
	case 'b':		/* background daemon*/
	    bflag = 1;
	    break;
	case 'd':		/* debug messages*/
	    dflag++;
	    break;
	case 'm':		/* MTU*/
		MTU = atoi(optarg);
		break;
	case 'p':		/* link protocol*/
	    linkprotocol = !strcmp(optarg, "eth")? LINK_ETHER :
			   !strcmp(optarg, "slip")? LINK_SLIP :
			   !strcmp(optarg, "cslip")? LINK_CSLIP:
			   -1;
	    if (linkprotocol < 0) usage();
	    break;
	case 's':		/* serial speed*/
	    baudrate = atol(optarg);
	    break;
	case 'l':		/* serial device*/
	    serdev = optarg;
	    break;
	case 'h':		/* help*/
	default:
	    usage();
	}
    }

    local_ip = in_aton(optind < argc? argv[optind++]: DEFAULT_IP);
    gateway_ip = in_aton(optind < argc? argv[optind++]: DEFAULT_GATEWAY);
    netmask_ip = in_aton(optind < argc? argv[optind++]: DEFAULT_NETMASK);

    /* must exit() in next two stages on error to reset kernel tcpdev_inuse to 0*/
    if ((tcpdevfd = tcpdev_init("/dev/tcpdev")) < 0)
	exit(1);

    if (linkprotocol == LINK_ETHER)
	intfd = deveth_init(ethdev);
    else
	intfd = slip_init(serdev, baudrate);
    if (intfd < 0)
	exit(2);

    printf("ktcp: ip %s, ", in_ntoa(local_ip));
    printf("gateway %s, ", in_ntoa(gateway_ip));
    printf("netmask %s\n", in_ntoa(netmask_ip));

    printf("ktcp: %s ", linknames[linkprotocol]);
    if (linkprotocol == LINK_ETHER)
	printf("%s", mac_ntoa((unsigned char *)&eth_local_addr));
    else printf("%s baud %lu", serdev, baudrate);
    printf(" mtu %u\n", MTU);

    signal(SIGHUP, catch);
    signal(SIGINT, catch);

    /* become daemon now that tcpdev_inuse race condition over*/
    if (bflag) {
	int fd;
	if (fork())
	    exit(0);
	close(0);
	/* redirect messages to console*/
	fd = open("/dev/console", O_WRONLY);
	dup2(fd, 1);		/* required for printf output*/
	dup2(fd, 2);
    }

    arp_init ();
    ip_init();
    icmp_init();
    tcp_init();
    netconf_init();

    ktcp_run();

    exit(0);
}

unsigned long in_aton(const char *str)
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

char *
in_ntoa(ipaddr_t in)
{
    register unsigned char *p;
    static char b[18];

    p = (unsigned char *)&in;
    sprintf(b, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
    return b;
}
