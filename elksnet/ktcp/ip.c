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
 * TODO : IP fragmentation and reassemply of fragmented IP packets
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ip.h"
#include "icmp.h"
#include "tcp.h"
#include "mylib.h"

#define IP_VERSION(s)	((s)->version_ihl>>4&0xf)
#define IP_IHL(s)	((s)->version_ihl&0xf)
#define IP_FLAGS(s)	((s)->frag_off>>13)

static char ipbuf[1024];

int ip_init()
{
    return 0;
}


__u16 ip_calc_chksum(data, len)
char *data;
int len;
{
    int i;
    __u32 sum;
    __u16 *p;
    
    p = (__u16 *)data;
    
    sum = 0;
    for( i = 0 ; i < len >> 1 ; i++ ){
	sum += *p;
	p++;
    }
    
    return ~((sum & 0xffff) + ((sum >> 16) & 0xffff));
}


void ip_print(head)
struct iphdr_s *head;
{
    __u8 *addr;
    int i;
    
    printf("Version/IHL :  %d/%d\n",IP_VERSION(head),IP_IHL(head));
    printf("TypeOfService/Length :  %d/%d\n",head->tos,ntohs(head->tot_len));
    printf("Id/flags/fragment offset :  %d/%d\n",head->id,head->frag_off);
    printf("ttl : %d\n",head->ttl);
    printf("Protocol : %d\n",head->protocol);

    addr = (__u8 *)&head->saddr;
    printf("saddr : %d.%d.%d.%d \n",addr[0],addr[1],addr[2],addr[3]);
    addr = (__u8 *)&head->daddr;
    printf("daddr : %d.%d.%d.%d \n",addr[0],addr[1],addr[2],addr[3]);
    
    printf("check sum = %d\n",ip_calc_chksum(head, 4 * IP_IHL(head)));
    
    addr = (__u8 *)head + 4 * IP_IHL(head);
    for( i = 0 ; i < ntohs(head->tot_len) - 20 ; i++ ) 
	printf("%x ",addr[i]);

    printf("\n");
}


void ip_recvpacket(packet, size)
char *packet;
int size;
{
    struct iphdr_s *iphdr;
    __u8 *addr;
    __u8 *data;
    
    iphdr = (struct iphdr_s *)packet;
    
    /*printf("IP: Got packet of size : %d \n",size,*packet);
    ip_print(iphdr);*/
    
    if(IP_VERSION(iphdr) != 4)
	return;
	
    if(IP_IHL(iphdr) < 5)
	return;

    if(ntohs(iphdr->tot_len) != size)
	return;
	
    data = packet + 4 * IP_IHL(iphdr);
    
    
    switch (iphdr->protocol) {
    case PROTO_ICMP:
		icmp_process(iphdr, data);
		break;
    case PROTO_TCP:
		tcp_process(iphdr);
		break;
    default:
	break;	
    }
    
}


void ip_sendpacket(packet, len, apair)
char *packet;
int len;
struct addr_pair *apair;
{
    struct iphdr_s *iph;
    __u16 tlen;
    
    
    iph = (struct iphdr_s *)&ipbuf;

    iph->version_ihl	= 0x45;
    
    tlen = 4 * IP_IHL(iph);
    
    iph->tos 		= 0;
    iph->tot_len	= htons(tlen + len);
    iph->id		= 0;
    iph->frag_off 	= 0;
    iph->ttl		= 64;
    iph->protocol	= apair->protocol;
    iph->daddr		= apair->daddr;
    iph->saddr		= apair->saddr;
    
    iph->check		= 0;
    iph->check		= ip_calc_chksum((char *)iph, tlen);    

    memcpy(&ipbuf[tlen], packet, len);
        
    /* "route" */
    if(iph->daddr == local_ip && iph->daddr == 0x0100007f){ /* 127.0.0.1 */
    	ip_recvpacket(iph, tlen + len);
    } else {
		slip_send(&ipbuf, tlen + len);   
	}
}

