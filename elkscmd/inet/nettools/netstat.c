/*
 * netstat is used to inspect the function of the ELKS networking
 *
 * Copyright (c) 2001 Harry Kalogirou <harkal@rainbow.cs.unipi.gr>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linuxmt/net.h>
#include <linuxmt/in.h>
#include <ktcp/tcp.h>
#include <ktcp/netconf.h>
#include <linuxmt/arpa/inet.h>

char tcp_states[11][13] = {
	"CLOSED",
	"LISTEN",
	"SYN_SENT",
	"SYN_RECEIVED",
	"ESTABLISHED",
	"FIN_WAIT_1",
	"FIN_WAIT_2",
	"CLOSE_WAIT",
	"CLOSING",
	"LAST_ACK",
	"TIME_WAIT"	
};

int s;
int ret,size;
struct sockaddr_in localadr,remaddr;
char buf[100];

int main(void)
{
    struct general_stats_s *gstats;
    struct cb_stats_s *cbstats;
    struct packet_stats_s *ns;
    struct stat_request_s sr;
    int i;
    char addr[16];
    __u8 *addrbytes;
	    
    if ( (s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	perror("socket error");
	exit(-1);
    }

    localadr.sin_family = AF_INET;
    localadr.sin_port = 0;
    localadr.sin_addr.s_addr = INADDR_ANY;  
    ret = bind(s, &localadr, sizeof(struct sockaddr_in));
    if ( ret == -1) {
	perror("bind error");
	exit(-1);
    }

    remaddr.sin_family = AF_INET;
    remaddr.sin_port = htons(NETCONF_PORT);
    remaddr.sin_addr.s_addr = 0;
    ret = connect(s, &remaddr, sizeof(struct sockaddr_in));
    if ( ret == -1) {
	perror("connect error");
	exit(-1);
    }

    sr.type = NS_GENERAL;
    write(s, &sr, sizeof(sr));	
    ret = read(s, buf, sizeof(buf));
    gstats = (struct general_stats_s *)buf;
    printf("Retransmit memory        : %d bytes\n", gstats->retrans_memory);
    printf("Number of control blocks : %d\n\n", gstats->cb_num);

    sr.type = NS_NETSTATS;
    write(s, &sr, sizeof(sr));
    ret = read(s, buf, sizeof(buf));
    ns = (struct packet_stats_s *)buf;
    printf("TCP Packets Rcv%8ld  TCP Packets Sent%8ld\n", ns->tcprcvcnt, ns->tcpsndcnt);
    printf("TCP Pkt Dropped%8ld  TCP Retransmits %8ld\n", ns->tcpdropcnt, ns->tcpretranscnt);
    printf("TCP Bad Chksum %8ld\n", ns->tcpbadchksum);
    printf("IP Packets Rcv %8ld  IP Packets Sent %8ld\n", ns->iprcvcnt, ns->ipsndcnt);
    printf("IP Bad Chksum  %8ld  IP Bad Headers  %8ld\n", ns->ipbadchksum, ns->ipbadhdr);
    printf("ICMP Pkts Rcv  %8ld  ICMP Packets Snt%8ld\n", ns->icmprcvcnt, ns->icmpsndcnt);
    printf("ETH Packets Rcv%8ld  ETH Packets Sent%8ld\n", ns->ethrcvcnt, ns->ethsndcnt);
    printf("ARP Reply Rcv  %8ld  ARP Reply Sent  %8ld\n", ns->arprcvreplycnt, ns->arpsndreplycnt);
    printf("ARP Request Rcv%8ld  ARP Request Sent%8ld\n", ns->arprcvreqcnt, ns->arpsndreqcnt);
    printf("ARP Cache Adds %8ld\n", ns->arpcacheadds);
    printf("\n");

    printf(" no        State    RTT lport        raddress  rport\n");
    printf("-----------------------------------------------------\n");
    sr.type = NS_CB;
    for (i = 0 ;; i++){
		sr.extra = i;
		write(s, &sr, sizeof(sr));	
		read(s, buf, sizeof(buf));
		cbstats = (struct cb_stats_s *)buf;
		if (cbstats->valid == 0)break;
		addrbytes = (__u8 *)&cbstats->remaddr;
		sprintf(addr,"%d.%d.%d.%d",addrbytes[0],addrbytes[1],addrbytes[2],addrbytes[3]);
		printf("%3d %12s %4dms", i+1, tcp_states[cbstats->state], cbstats->rtt);
		printf(" %5u %15s  %5u\n", cbstats->localport, addr, cbstats->remport);
    }
}

