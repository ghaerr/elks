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

/*
 * TODO : IP fragmentation and reassembly of fragmented IP packets
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "ip.h"
#include "tcp.h"
#include "tcpdev.h"
#include "icmp.h"
#include "slip.h"
#include "deveth.h"
#include "arp.h"
#include "netconf.h"

static unsigned char ipbuf[IP_BUFSIZ];

int ip_init(void)
{
    return 0;
}

__u16 ip_calc_chksum(char *data, int len)
{
    __u32 sum = 0;
    __u16 *p = (__u16 *) data;

    for (; len > 1 ; len -= 2)
	sum += *p++;

    if (len == 1)
	sum += (__u16)(*(__u8 *)p);

    while (sum >> 16)
	sum = (sum & 0xffff) + (sum >> 16);
    return ~(__u16)sum;
}

/*** __u16 ip_calc_chksum(char *data, int len)
	.text
	.globl _ip_calc_chksum
_ip_calc_chksum:

	push	bp
	mov	bp,sp
	push	di

	mov	cx, 6[bp]
	shr	cx, 1
	shr	cx, 1
	mov	di, 4[bp]
	xor	ax, ax
loop1:
	adc	ax, [di]
	inc di
	inc di
	adc	ax, [di]
	inc di
	inc di

        loop	loop1

	adc	ax, 0
	not	ax

	pop di
	pop bp

	ret
***/

static void ip_print(struct iphdr_s *head, int size)
{
#if DEBUG_IP
    debug_ip("IP: %s ", size? "recv": "send");
    debug_ip("%s -> ", in_ntoa(head->saddr));
    debug_ip("%s ", in_ntoa(head->daddr));
    debug_ip("v%d hl:%d ", IP_VERSION(head), IP_HLEN(head));
    debug_ip("tos:%d len:%u ", head->tos, ntohs(head->tot_len));
    debug_ip("id:%u ", ntohs(head->id));
    debug_ip("fo:%x ", ntohs(head->frag_off));	//FIXME add FM_ flags
    debug_ip("chk:%x ", ip_calc_chksum((char *)head, 4 * IP_HLEN(head)));
    debug_ip("ttl:%d ", head->ttl);
    debug_ip("prot:%d\n", head->protocol);
#if DEBUG_IP > 1
    debug_ip("   opts:");
    int i;
    unsigned char *addr = (__u8 *)head + 4 * IP_HLEN(head);
    for (i = 0 ; i < ntohs(head->tot_len) - 20 ; i++ )
	debug_ip("%02x ",addr[i]);
    debug_ip("\n");
#endif
#endif
}

void ip_recvpacket(unsigned char *packet,int size)
{
    register struct iphdr_s *iphdr = (struct iphdr_s *)packet;
    int len;
    unsigned char *data;

    ip_print(iphdr, size);

    if (IP_VERSION(iphdr) != 4){
        debug_ip("IP: Bad IP version\n");
	netstats.ipbadhdr++;
	return;
    }

    len = IP_HLEN(iphdr) * 4;
    if (len < 20) {
        debug_ip("IP: Bad HLEN\n");
	netstats.ipbadhdr++;
	return;
    }

    if (ip_calc_chksum((char *)iphdr, len)) {
	printf("IP: BAD CHKSUM (%x) len %d\n", ip_calc_chksum((char *)iphdr, len), len);
	netstats.ipbadchksum++;
	return;
    }

    switch (iphdr->protocol) {
    case PROTO_ICMP:
        debug_ip("IP: recv icmp packet\n");
	data = packet + 4 * IP_HLEN(iphdr);
	icmp_process(iphdr, data);
	netstats.icmprcvcnt++;
	break;

    case PROTO_TCP:
        debug_ip("IP: recv tcp packet\n");
	tcp_process(iphdr);
	netstats.tcprcvcnt++;
	break;
    }
    netstats.iprcvcnt++;
}

void ip_sendpacket(unsigned char *packet, int len, struct addr_pair *apair)
{
    /*
     * save space for possible ethernet header before ip packet
     * in eth_route()/eth_sendpacket()
     */
    register struct iphdr_s *iph = (struct iphdr_s *)(ipbuf + sizeof(struct ip_ll));
    int iphdrlen;
    static __u16 nextID = 1;

    /* ip layer*/
    iph->version_ihl	= 0x45;
    iphdrlen		= 4 * IP_HLEN(iph);
    iph->tos 		= 0;
    iph->tot_len	= htons(iphdrlen + len);
    iph->id		= htons(nextID); nextID++;
    iph->frag_off 	= 0;
    iph->ttl		= 64;
    iph->saddr		= apair->saddr;
    iph->daddr		= apair->daddr;
    iph->protocol	= apair->protocol;
    iph->check		= 0;
    iph->check		= ip_calc_chksum((char *)iph, iphdrlen);
    memcpy((char *)iph + iphdrlen, packet, len); //FIXME don't copy, fixup in upper layer

    ip_print(iph, 0);
#if DEBUG_TCP
    struct iptcp_s iptcp;
    iptcp.iph = iph;
    iptcp.tcph = (struct tcphdr_s *)(((char *)iph) + 4 * IP_HLEN(iph));
    iptcp.tcplen = ntohs(iph->tot_len) - 4 * IP_HLEN(iph);
    tcp_print(&iptcp, 0);
#endif

    /* route packet using src and dst address*/
    ip_route((unsigned char *)iph, iphdrlen + len, apair);
    netstats.ipsndcnt++;
}

void ip_route(unsigned char *packet, int len, struct addr_pair *apair)
{
    ipaddr_t ip_addr;

    /* determine if packet is routed to localhost, route right back to us*/
    if (apair->saddr  == local_ip && apair->daddr == local_ip) {
	debug_ip("ip: route localhost\n");
	ip_recvpacket(packet, len);
	return;
    }

    /* route based on netmask. FIXME: interface never changed; ignored for SLIP/CSLIP interface*/
    if ((local_ip & netmask_ip) != (apair->daddr & netmask_ip))
        /* Not on the same local network */
        /* Route to the gateway as local destination */
        ip_addr = gateway_ip;
    else
        /* On the same local network */
        /* Route to the local destination */
        ip_addr = apair->daddr;

    if (linkprotocol == LINK_ETHER)
	eth_route(packet, len, ip_addr);
    else
	slip_send(packet, len);
}
