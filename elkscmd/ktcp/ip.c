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
#include "netorder.h"

#if 0
#define IP_VERSION(s)	((s)->version_ihl>>4&0xf)
#define IP_IHL(s)	((s)->version_ihl&0xf)
#define IP_FLAGS(s)	((s)->frag_off>>13)
#endif

/*#define DEBUG*/
#define USE_ASM

static char ipbuf[1024];

int ip_init(void)
{
    return 0;
}

#ifndef USE_ASM
__u16 ip_calc_chksum(char *data, int len)
{
    __u32 sum = 0;
    int i;
    __u16 *p = (__u16 *) data;
    
    len >>= 1;
    
    for (i=0; i < len ; i++){
	sum += *p++;
    }
    
    return ~((sum & 0xffff) + ((sum >> 16) & 0xffff));
}
#else
#asm
/*__u16 ip_calc_chksum(char *data, int len)*/
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
#endasm
	

#endif

void ip_print(struct iphdr_s *head)
{
#ifdef DEBUG
    int i;
    __u8 *addr;
    
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
    for ( i = 0 ; i < ntohs(head->tot_len) - 20 ; i++ ) 
	printf("%x ",addr[i]);

    printf("\n");
#endif
}

void ip_recvpacket(char *packet,int size)
{
    struct iphdr_s *iphdr;
    __u8 *addr, *data;

    iphdr = (struct iphdr_s *)packet;

    /*printf("IP: Got packet of size : %d \n",size,*packet);
    ip_print(iphdr);*/

    if (IP_VERSION(iphdr) != 4){
#ifdef DEBUG
        printf("IP : Bad IP version\n");
#endif
	return;
    }

    if (IP_IHL(iphdr) < 5){
#ifdef DEBUG
        printf("IP : Bad IHL\n");
#endif
	return;
    }
    
    data = packet + 4 * IP_IHL(iphdr);

    switch (iphdr->protocol) {
    case PROTO_ICMP:
#ifdef DEBUG
                printf("IP : ICMP packet\n");
#endif

		icmp_process(iphdr, data);
		break;

    case PROTO_TCP:
#ifdef DEBUG
                printf("IP : TCP packet\n");
#endif
		tcp_process(iphdr);
		break;

    default:
	break;	

    }
}

void ip_sendpacket(char *packet,int len,struct addr_pair *apair)
{
    struct iphdr_s *iph = (struct iphdr_s *)&ipbuf;
    __u16 tlen;

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
    if (iph->daddr == local_ip && iph->daddr == 0x0100007f) {
	/* 127.0.0.1 */
	/* TODO */
    } else
	slip_send(&ipbuf, tlen + len);   
}
