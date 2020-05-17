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
#include "config.h"
#include "ip.h"
#include "icmp.h"
#include <linuxmt/arpa/inet.h>

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
    case ICMP_ECHO_REQUEST:
	printf("icmp: PING from %s (len %d id %d seqnum %d)\n",
	    in_ntoa(iph->saddr), ntohs(iph->tot_len), ntohs(ep->id), ntohs(ep->seqnum));
	ep->type = ICMP_ECHO_REPLY;
	ep->code = 0;
	/* return received id and seqnum*/
	ep->chksum = 0;
	ep->chksum = ip_calc_chksum((char *)ep, sizeof(struct icmp_echo_s));

	apair.daddr = iph->saddr;
	apair.saddr = iph->daddr;
	apair.protocol = PROTO_ICMP;
	len = ntohs(iph->tot_len) - IP_IHL(iph);
	ip_sendpacket(packet, len, &apair);
	break;
   case ICMP_ECHO_DESTUNREACHABLE:
	printf("icmp: destination unreachable code %d from %s\n",
		ep->code, in_ntoa(iph->saddr));
	break;
    default:
	debug_ip("icmp: unrecognized ICMP request %d\n", ep->type);
	break;
    }
}
