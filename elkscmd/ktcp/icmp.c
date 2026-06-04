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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include "config.h"
#include "ip.h"
#include "tcp.h"
#include "icmp.h"
#include "tcp_cb.h"
#include "tcpdev.h"
#include "netconf.h"

/* Send a raw ICMP echo request.
 * NOTE: ip_sendpacket() prepends its own IP header, so buf contains only
 * the ICMP header + payload — do NOT embed a second IP header here. */
void icmp_send_echo(ipaddr_t target_ip, unsigned short id, unsigned short seq, unsigned long timestamp)
{
    struct addr_pair apair;
    int len = sizeof(struct icmp_echo_s) + 4;
    unsigned char buf[sizeof(struct icmp_echo_s) + 4];
    struct icmp_echo_s *icmp = (struct icmp_echo_s *)buf;
    unsigned long *payload = (unsigned long *)(icmp + 1);	/* 32-bit timestamp on ia16 (int=16, long=32) */

    icmp->type = ICMP_TYPE_ECHO_REQ;
    icmp->code = 0;
    icmp->chksum = 0;
    icmp->id = htons(id);
    icmp->seqnum = htons(seq);
    *payload = timestamp;

    icmp->chksum = ip_calc_chksum((char *)icmp, len);

    apair.daddr = target_ip;
    apair.saddr = local_ip;
    apair.protocol = PROTO_ICMP;
    ip_sendpacket(buf, len, &apair, NULL);
    netstats.icmpsndcnt++;
}

int icmp_init(void)
{
    return 0;
}

void icmp_process(struct iphdr_s *iph, unsigned char *packet)
{
    struct icmp_echo_s *ep;
    struct addr_pair apair;

    struct icmp_dest_unreachable_s *dp;
    struct iphdr_s *dpip;
    struct tcphdr_s *dptcp;
    struct tcpcb_list_s *cbnode;

    int len;

    switch (packet[0]){
    case ICMP_TYPE_ECHO_REQ:
	ep = (struct icmp_echo_s *)packet;
	len = ntohs(iph->tot_len) - 4 * IP_HLEN(iph);
	debug_ip("icmp: PING from %s (len %d id %u seqnum %u)\n",
	    in_ntoa(iph->saddr), len, ntohs(ep->id), ntohs(ep->seqnum));

	ep->type = ICMP_TYPE_ECHO_REPL;
	ep->code = 0;
	/* return received id, seqnum and extra data*/
	ep->chksum = 0;
	ep->chksum = ip_calc_chksum((char *)ep, len);

	apair.daddr = iph->saddr;
	apair.saddr = iph->daddr;
	apair.protocol = PROTO_ICMP;
	ip_sendpacket(packet, len, &apair, NULL);
	netstats.icmpsndcnt++;
	break;
    case ICMP_TYPE_ECHO_REPL:
	/* Echo Reply — forward to pending netconf client (needed so ping(1) gets its reply via netconf protocol) */
	if (pending_icmp_cb) {
	    __u32 *ts = (__u32 *)(packet + sizeof(struct icmp_echo_s));
	    netconf_icmp_reply(pending_icmp_cb, *ts, iph->ttl);
	}
	break;
    case ICMP_TYPE_DST_UNRCH:
	dp = (struct icmp_dest_unreachable_s *)packet;
	dpip = (struct iphdr_s *)dp->iphdr;
	len = 4 * IP_HLEN(dpip);
	dptcp = (struct tcphdr_s *)(dp->iphdr + len);
	printf("icmp: destination unreachable code %d from %s\n",
		dp->code, in_ntoa(iph->saddr));
	debug_ip("icmp: src %s:%u ", in_ntoa(dpip->saddr), ntohs(dptcp->sport));
	debug_ip("dst %s:%u\n", in_ntoa(dpip->daddr), ntohs(dptcp->dport));
	cbnode = tcpcb_find(dpip->daddr, ntohs(dptcp->sport), ntohs(dptcp->dport));
	if (cbnode) {
	    struct tcpcb_s *cb = &cbnode->tcpcb;
	    int err = (dp->code == 1)? -EHOSTUNREACH :
		      (dp->code >= 9)? -ECONNREFUSED : -ENETUNREACH;

	    notify_sock(cb->sock, TDT_CONNECT, err);
	    tcpcb_remove_cb(cb);	/* deallocate */
	} else debug_ip("icmp: Connection not found\n");
	break;
    default:
	printf("icmp: unrecognized ICMP type %d\n", packet[0]);
	break;
    }
}
