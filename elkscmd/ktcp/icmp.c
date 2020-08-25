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
#include <fcntl.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "config.h"
#include "ip.h"
#include "icmp.h"
#include "netconf.h"

int icmp_init(void)
{
    return 0;
}

void icmp_process(struct iphdr_s *iph,unsigned char *packet)
{
    struct icmp_echo_s *ep = (struct icmp_echo_s *)packet;
    struct addr_pair apair;
    int len;

    switch (packet[0]){
    case ICMP_TYPE_ECHO_REQ:
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
	ip_sendpacket(packet, len, &apair);
	netstats.icmpsndcnt++;
	break;
   case ICMP_TYPE_DST_UNRCH:
	printf("icmp: destination unreachable code %d from %s\n",
		ep->code, in_ntoa(iph->saddr));
	break;
    default:
	debug_ip("icmp: unrecognized ICMP request %d\n", ep->type);
	break;
    }
}
