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
#include "ip.h"
#include "tcp.h"
#include "mylib.h"
#include "timer.h"


static struct tcp_retrans_list_s *retrans_list;
char buf[128];

__u16 tcp_chksum(h)
struct iptcp_s *h;
{
	__u32 sum;
	__u16 *data;
	__u16 len;

	/* I will optimize this for 8086 later */
	data = (__u16 *)h->tcph;
	len = h->tcplen;
	sum = 0;
	sum += htons(len);

	for(; len > 1 ; len -= 2)
		sum += *data++;
		
	if(len == 1)
		sum += (__u16)(*(__u8 *)data);
		
	sum += h->iph->saddr & 0xffff;
	sum += (h->iph->saddr >> 16) & 0xffff;
	sum += h->iph->daddr & 0xffff;
	sum += (h->iph->daddr >> 16) & 0xffff;
	sum += htons((__u16)6);
	
	return ~((sum & 0xffff) + ((sum >> 16) & 0xffff));
}

__u16 tcp_chksumraw(h,saddr,daddr,len)
struct tcphdr_s *h;
__u32	saddr;
__u32	daddr;
__u16	len;
{
	__u32 sum;
	__u16 *data;

	/* I will optimize this for 8086 later */
	data = (__u16 *)h;
	sum = 0;
	sum += htons(len);

	for(; len > 1 ; len -= 2)
		sum += *data++;
		
	if(len == 1)
		sum += (__u16)(*(__u8 *)data);
		
	sum += saddr & 0xffff;
	sum += (saddr >> 16) & 0xffff;
	sum += daddr & 0xffff;
	sum += (daddr >> 16) & 0xffff;
	sum += htons((__u16)6);
	
	return ~((sum & 0xffff) + ((sum >> 16) & 0xffff));
}

struct tcp_retrans_list_s *rmv_from_retrans(n)
struct tcp_retrans_list_s *n;
{
	struct tcp_retrans_list_s *next;
	
	next = n->next;
	
	tcp_timeruse--;
	tcp_retrans_memory -= n->len;
	if(n->prev){
		n->prev->next = next;
	} else {
		/* Head update */
		n = next;
		n->prev = NULL;
		free(retrans_list->tcph);
		free(retrans_list);
		retrans_list = n;

		return n;
	}
	
	if(next){
		next->prev = n->prev;
	}
	
	free(n->tcph);
	free(n);
	return next;
}

void rmv_all_retrans(lcb)
struct tcpcb_list_s *lcb;
{
	struct tcp_retrans_list_s *n;

	n = retrans_list;
	
	while(n != NULL){
		if(n->cb == &lcb->tcpcb){
			n = rmv_from_retrans(n);
		} else {
			n = n->next;	
		}
	}
	
}

void add_for_retrans(cb, th, len, apair)
struct tcpcb_s	*cb;
struct tcphdr_s	*th;
__u16 len;
struct addr_pair *apair;
{
	struct tcp_retrans_list_s *n;
	
	if(((th->flags & 0xff00) == TF_ACK) && cb->datalen == 0)
		return;
		
	n = (struct tcp_retrans_list_s *)malloc(sizeof(struct tcp_retrans_list_s));
	if(n == NULL){
		printf("ERROR : Out of memory\n");
		/* In this case has to be avoided */
		exit(0);
		return;
	}
	
	/* So thet select in ktcp.c blocks for
	 * a specified time only */
	tcp_timeruse++;
	
	/* Link it to the list */
	if(retrans_list){
		n->next = retrans_list;
		n->prev = NULL;
		retrans_list->prev = n;
		retrans_list = n;
	} else {
		retrans_list = n;
		n->next = NULL;
		n->prev = NULL;
	}
	
	n->cb = cb;
	n->tcph = (struct tcphdr_s *)malloc(len);
	if(!n->tcph){
		printf("Out of memory\n");
		exit(0);	
	}
	n->len = len;
	tcp_retrans_memory += len;
	memcpy(n->tcph, th, len);
	memcpy(&n->apair, apair, sizeof(struct addr_pair));
	n->retrans_num = 0;
	n->first_trans = Now;
	
	n->rto = cb->rtt << 1;
	n->next_retrans = Now + n->rto;
}

void tcp_reoutput(n)
struct tcp_retrans_list_s *n;
{
	n->retrans_num ++;
	n->rto *= 2;
	n->next_retrans = Now + n->rto;
	
	ip_sendpacket(n->tcph, n->len, &n->apair);
}


int tcp_retrans()
{
	struct tcp_retrans_list_s *n;
	int datalen;
	int rtt;

/*	printf("Retrans buffers : %d ", tcp_timeruse);*/
	n = retrans_list;
	while(n != NULL){
		datalen = n->len - TCP_DATAOFF(n->tcph);
/*		printf("retrans : %lu %lu",ntohl(n->tcph->seqnum) + datalen,n->cb->send_una);*/
		if(SEQ_LEQ(ntohl(n->tcph->seqnum) + datalen ,n->cb->send_una)){
			if(n->retrans_num == 0){
				rtt = Now - n->first_trans;
				if(rtt > 0){
					n->cb->rtt = (TCP_RTT_ALPHA * n->cb->rtt + (100 - TCP_RTT_ALPHA) * rtt) / 100;
				}
			}
			n = rmv_from_retrans(n);
/*			printf(" remove\n");*/
			continue;
		}
/*		printf("not\n");*/
		if(TIME_GEQ(Now, n->next_retrans)){
			tcp_reoutput(n);
			return;
		}
		n = n->next;
	}
}

void tcp_output(cb)
struct tcpcb_s	*cb;
{
	struct tcphdr_s *th;
	struct addr_pair apair;
	__u16 len;
	
	th = (struct tcphdr_s *)&buf;
	
	th->sport = htons(cb->localport);
	th->dport = htons(cb->remport);
	th->seqnum = htonl(cb->send_nxt);
	th->acknum = htonl(cb->rcv_nxt);
	
	cb->send_nxt += cb->datalen;
	
	th->window = htons((__u16)CB_BUF_SPACE(cb));
	th->urgpnt = 0;
	th->flags = cb->flags;	
	
	TCP_SETHDRSIZE(th, 5);
	
	len = cb->datalen + 20; /* FIXME : Options */
	memcpy(&th->options, cb->data, cb->datalen); /* Since no options yet */

	th->chksum = 0;
	th->chksum = tcp_chksumraw(th, cb->localaddr, cb->remaddr, len);
		
	apair.saddr = cb->localaddr;
	apair.daddr = cb->remaddr;
	apair.protocol = PROTO_TCP;
	
	add_for_retrans(cb, th, len, &apair);
	ip_sendpacket(th, len, &apair);
}

	
