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

#define ICMP_ECHO_REPLY		0

int icmp_init(void)
{
    return 0;
}

void icmp_process(struct iphdr_s *iph,unsigned char *packet)
{
    struct addr_pair apair;
    int len;

    switch (packet[0]){
	case 8: /* ICMP_ECHO */
	    debug_ip("icmp: PING from %s\n", in_ntoa(iph->saddr));
	    apair.daddr = iph->saddr;
	    apair.saddr = iph->daddr;
	    apair.protocol = PROTO_ICMP;
	    len = ntohs(iph->tot_len) - 20;	/* Do this right */
	    packet[0] = ICMP_ECHO_REPLY;
	    ip_sendpacket(packet, len, &apair);
	    break;
	default:
	    debug_ip("icmp: unregocognized ICMP request %d\n", packet[0]);
	    break;
    }
}
