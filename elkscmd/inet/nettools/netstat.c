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
    unsigned int retrans_mem;
    char addr[16];
    __u8 *addrbytes;
	    
    if ( (s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	perror("socket error");
	exit(-1);
    }

    localadr.sin_family = AF_INET;
    localadr.sin_port = 0;
    localadr.sin_addr.s_addr = INADDR_ANY;  
    ret = bind(s, (struct sockaddr *)&localadr, sizeof(struct sockaddr_in));
    if ( ret == -1) {
	perror("bind error");
	exit(-1);
    }

    remaddr.sin_family = AF_INET;
    remaddr.sin_port = htons(NETCONF_PORT);
    remaddr.sin_addr.s_addr = 0;
    ret = connect(s, (struct sockaddr *)&remaddr, sizeof(struct sockaddr_in));
    if ( ret == -1) {
	perror("connect error");
	exit(-1);
    }

    sr.type = NS_GENERAL;
    write(s, &sr, sizeof(sr));	
    ret = read(s, buf, sizeof(buf));
    gstats = (struct general_stats_s *)buf;
    retrans_mem = gstats->retrans_memory;

    sr.type = NS_NETSTATS;
    write(s, &sr, sizeof(sr));
    ret = read(s, buf, sizeof(buf));
    ns = (struct packet_stats_s *)buf;
    printf("----- Received ---------  ----- Sent -------------\n");
    printf("TCP Packets      %7lu  TCP Packets      %7lu\n", ns->tcprcvcnt, ns->tcpsndcnt);
    printf("TCP Dropped      %7lu  TCP Retransmits  %7lu\n", ns->tcpdropcnt, ns->tcpretranscnt);
    printf("TCP Bad Checksum %7lu  TCP Retrans Memory%6u\n", ns->tcpbadchksum, retrans_mem);
    printf("IP Packets       %7lu  IP Packets       %7lu\n", ns->iprcvcnt, ns->ipsndcnt);
    printf("IP Bad Checksum  %7lu  IP Bad Headers   %7lu\n", ns->ipbadchksum, ns->ipbadhdr);
    printf("ICMP Packets     %7lu  ICMP Packets     %7lu\n", ns->icmprcvcnt, ns->icmpsndcnt);
    printf("SLIP Packets     %7lu  SLIP Packets     %7lu\n", ns->sliprcvcnt, ns->slipsndcnt);
    printf("ETH Packets      %7lu  ETH Packets      %7lu\n", ns->ethrcvcnt, ns->ethsndcnt);
    printf("ARP Replies (rcv)%7lu  ARP Requests(snd)%7lu\n", ns->arprcvreplycnt, ns->arpsndreqcnt);
    printf("ARP Requests(rcv)%7lu  ARP Replies (snd)%7lu\n", ns->arprcvreqcnt, ns->arpsndreplycnt);
    printf("ARP Cache Adds   %7lu\n", ns->arpcacheadds);
    printf("\n");

    printf(" No        State    RTT lport        raddress  rport\n");
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
