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
#include <errno.h>
#include <arpa/inet.h>
#include "slip.h"
#include "tcp.h"
#include "tcp_output.h"
#include "tcp_cb.h"
#include "tcpdev.h"
#include "timer.h"
#include "ip.h"
#include "icmp.h"
#include "netconf.h"
#include "deveth.h"
#include "arp.h"

ipaddr_t local_ip;
ipaddr_t gateway_ip;
ipaddr_t netmask_ip;

#define DEFAULT_IP		"10.0.2.15"
#define DEFAULT_GATEWAY		"10.0.2.2"
#define DEFAULT_NETMASK		"255.255.255.0"

/* defaults*/
int linkprotocol = 	LINK_ETHER;
char ethdev[] = 	"/dev/eth";
char *serdev = 		"/dev/ttyS0";
speed_t baudrate = 	57600;

int dflag;
unsigned int MTU;
static int intfd;	/* interface fd*/

// rename		timer			function called when active
//			----------------	-------------------------------------
// tcp_timeruse		timer_retrans		tcp_retrans
// cbs_in_time_wait	timer_time_wait		tcp_expire_timeouts
// cbs_in_user_wait	timer_close_wait	tcp_expire_timeouts
// tcpcb_need_push				tcpcb_push_data -> notify_data_avail

int tcp_timeruse;		/* retrans timer active, call tcp_retrans */
int cbs_in_time_wait;		/* time_wait timer active, call tcp_expire_timeouts */
int cbs_in_user_timeout;	/* fin_wait/closing/last_ack active, call " */
int tcpcb_need_push;		/* push required, tcpcb_push_data/call notify_data_avail */
int tcp_retrans_memory;		/* total retransmit memory in use*/

void ktcp_run(void)
{
    fd_set fdset;
    struct timeval timeint, *tv;
    int count;
    int loopagain = 0;

    while (1) {
	if (tcp_timeruse > 0 || tcpcb_need_push > 0 || loopagain ||
	    cbs_in_time_wait > 0 || cbs_in_user_timeout > 0) {

	    //printf("tcp: timer %d needpush %d timewait %d usertime %d\n", tcp_timeruse,
		//tcpcb_need_push, cbs_in_time_wait, cbs_in_user_timeout);

	    /* don't wait long if data needs pushing to tcpdev */
	    if (tcpcb_need_push || loopagain) {
		timeint.tv_sec  = 0;
		timeint.tv_usec = tcpcb_need_push? 1000: 0;	/* 1msec */
	    } else {
		timeint.tv_sec  = 1;
		timeint.tv_usec = 0;
	    }
	    tv = &timeint;
	} else {
	    tv = NULL;		/* no timeout if no timers active or push needed */
	}

	FD_ZERO(&fdset);
	FD_SET(intfd, &fdset);
	FD_SET(tcpdevfd, &fdset);
	count = select(intfd > tcpdevfd ? intfd + 1 : tcpdevfd + 1, &fdset, NULL, NULL, tv);
	if (count < 0) {
		if (errno == EINTR)
			continue;
		printf("ktcp: select failed errno %d\n", errno);
		return;
	}

	Now = timer_get_time();

	/* expire timeouts*/
	if (cbs_in_time_wait > 0 || cbs_in_user_timeout > 0) {
	    debug_tcp("tcp: time_wait %d user_timeout %d\n",
		cbs_in_time_wait, cbs_in_user_timeout);
	    tcpcb_expire_timeouts();
	}

	/* always push data*/
	//if (tcpcb_need_push > 0)
	    tcpcb_push_data();

	loopagain = 0;

	/* process received packets*/
	if (FD_ISSET(intfd, &fdset)) {
		if (linkprotocol == LINK_ETHER)
			eth_process();
		else slip_process();
		loopagain = 1;
	}

	/* process application socket actions*/
	if (FD_ISSET(tcpdevfd, &fdset)) {
		tcpdev_process();
		loopagain = 1;
	}

	/* check for expired retrans packets and free them*/
	if (tcp_timeruse > 0)
		tcp_retrans_expire();

	/* read all packets and sockets before handling retransmits*/
	if (loopagain)
		continue;

	/* check for retransmit packets required*/
	if (tcp_timeruse > 0)
		tcp_retrans_retransmit();

	tcpcb_printall();
    }
}

#if USE_DEBUG_EVENT
static int dprintf_on = DEBUG_STARTDEF;

void debug_toggle(int sig)
{
	dprintf_on = !dprintf_on;
	printf("\nktcp: debug %s\n", dprintf_on? "on": "off");
	signal(SIGURG, debug_toggle);
}

void dprintf(const char *fmt, ...)
{
	va_list ptr;

	if (!dprintf_on)
		return;
	va_start(ptr, fmt);
	vfprintf(stdout, fmt, ptr);
	va_end(ptr);
}
#endif

void catch(int sig)
{
    printf("ktcp: exiting on signal %d\n", sig);
	exit(1);
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
    int mtu = 0;
    char *p;
    static char *linknames[3] = { "ethernet", "slip", "cslip" };

    while ((ch = getopt(argc, argv, "bdm:p:s:l:")) != -1) {
	switch (ch) {
	case 'b':		/* background daemon*/
	    bflag = 1;
	    break;
	case 'd':		/* debug messages*/
	    dflag++;
	    break;
	case 'm':		/* MTU*/
		mtu = (int)atol(optarg);
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

    /*
     * Default IP, gateway and netmask set by env variables in
     * /bootopts or /etc/profile. They can be IP addresses or
     * names in /etc/hosts.
     */
    char *default_ip = (p=getenv("HOSTNAME"))? p: DEFAULT_IP;
    char *default_gateway = (p=getenv("GATEWAY"))? p: DEFAULT_GATEWAY;
    char *default_netmask = (p=getenv("NETMASK"))? p: DEFAULT_NETMASK;
    local_ip = in_gethostbyname(optind < argc? argv[optind++]: default_ip);
    gateway_ip = in_gethostbyname(optind < argc? argv[optind++]: default_gateway);
    netmask_ip = in_gethostbyname(optind < argc? argv[optind++]: default_netmask);
    MTU = mtu ? mtu : (linkprotocol == LINK_ETHER ? ETH_MTU : SLIP_MTU);

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
	printf("%s", mac_ntoa(eth_local_addr));
    else printf("%s baud %lu", serdev, baudrate);
    printf(" mtu %u\n", MTU);

    signal(SIGHUP, catch);
    signal(SIGINT, catch);
#if USE_DEBUG_EVENT
    signal(SIGURG, debug_toggle);
#endif

    /* become daemon now that tcpdev_inuse race condition over*/
    if (bflag) {
	int fd, ret;
	if ((ret = fork()) == -1) {
	    printf("ktcp: Can't fork to become daemon\n");
	    exit(1);
	}
	if (ret) exit(0);
	close(0);
	/* redirect messages to console*/
	fd = open("/dev/console", O_WRONLY);
	dup2(fd, 1);		/* required for printf output*/
	dup2(fd, 2);
	setsid();
    }

    arp_init();
    ip_init();
    icmp_init();
    tcp_init();
    netconf_init();

    ktcp_run();

    return 0;
}
