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
#include "slip.h"
#include "icmp.h"
#include "ip.h"
#include <linuxmt/arpa/inet.h>

int icmp_init(void)
{
    return 0;
}

void icmp_process(struct iphdr_s *iph,char *packet)
{
    struct addr_pair apair;
    int len;

    switch (packet[0]){
	case 8: /* ICMP_ECHO */
	    apair.daddr = iph->saddr;
	    apair.saddr = iph->daddr;
	    apair.protocol = PROTO_ICMP;
	    len = ntohs(iph->tot_len) - 20;	/* Do this right */

/*	Set the type to ICMP_ECHO_REPLY 
 */
	    packet[0] = 0;
	    ip_sendpacket(packet, len, &apair);
	    break;
    }
}
