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
#include <linuxmt/errno.h>
#include <linuxmt/net.h>
#include "slip.h"
#include "ip.h"
#include "tcp.h"
#include "mylib.h"

extern int cbs_in_time_wait;

void tcp_print(head)
struct iptcp_s *head;
{
/*	printf("TCP header\n");
	printf("sport : %d dport :%d \n",ntohs(head->tcph->sport),ntohs(head->tcph->dport));
	printf("seq : %lu ack : %lu \n",ntohl(head->tcph->seqnum), ntohl(head->tcph->acknum));
	printf("flags :%d\n",head->tcph->flags);
	printf("window : %d urgpnt : %d\n",ntohs(head->tcph->window),head->tcph->urgpnt);	
	printf("chksum : %d\n",tcp_chksum(head));*/
}

int tcp_init()
{
	struct tcpcb_list_s *n;
	tcpcb_init();
	cbs_in_time_wait = 0;
	tcp_retrans_memory = 0;

    return 0;
}

long choose_seq()
{
	static num = 200;
	return num++;
}

void tcp_send_fin(cb)
struct tcpcb_s *cb;
{
	cb->flags = TF_FIN|TF_ACK;
	cb->datalen = 0;	
	tcp_output(cb);
	cb->send_nxt++;
}

void tcp_send_reset(cb)
struct tcpcb_s *cb;
{
	cb->flags = TF_RST;
	cb->datalen = 0;
	tcp_output(cb);	
}

void tcp_send_ack(cb)
struct tcpcb_s *cb;
{
	cb->flags = TF_ACK;
	cb->datalen = 0;
	tcp_output(cb);	
}

void tcp_connect(cb)
struct tcpcb_s *cb;	
{

	cb->iss = choose_seq();
	cb->send_nxt = cb->iss;
	cb->send_una = cb->iss;
		
	cb->state = TS_SYN_SENT;
	cb->flags = TF_SYN;

	cb->datalen = 0;	

	tcp_output(cb);
	
}

void tcp_syn_sent(iptcp, cb)
struct iptcp_s *iptcp;
struct tcpcb_s *cb;	
{
	struct tcphdr_s *h;
	
	h = iptcp->tcph;
	
	cb->seg_seq = ntohl(h->seqnum);
	cb->seg_ack = ntohl(h->acknum);
	
	if(h->flags & TF_RST){
		retval_to_sock(cb->sock, -ECONNREFUSED);
		return;
	}
	
	if(h->flags & (TF_SYN|TF_ACK)){
		if(cb->seg_ack != cb->send_una + 1){
			/* Send RESET */
			cb->send_nxt = h->acknum;
			cb->state = TF_RST;
		
			cb->datalen = 0;	
			tcp_output(cb);
			return;
		}
		
		cb->irs = cb->seg_seq;
		cb->rcv_nxt = cb->irs+1;
		cb->rcv_wnd = ntohs(h->window);
	
		cb->send_nxt++;
		cb->send_una++;
		cb->state = TS_ESTABLISHED;
		cb->flags = TF_ACK;
	
		cb->datalen = 0;	
		tcp_output(cb);
		
		retval_to_sock(cb->sock, 0);
		
		return;
	}
}

void tcp_listen(iptcp, lcb)
struct iptcp_s *iptcp;
struct tcpcb_s *lcb;	
{
	struct tcpcb_list_s *n;
	struct tcpcb_s	*cb;
	struct tcphdr_s *h;
	
	h = iptcp->tcph;
	
	if(h->flags & TF_RST)
		return;
		
	if(h->flags & TF_ACK){
/*		printf("Implement me ACK in listen\n");*/ 
	}
	
	n = tcpcb_clone(lcb);
	cb = &n->tcpcb;
	cb->unaccepted = 1; /* Mark as unaccepted */
	
	cb->seg_seq = ntohl(h->seqnum);
	cb->seg_ack = ntohl(h->acknum);
	
	if(!(h->flags & TF_SYN)){
/*		printf("not sync message in listen state\n");*/
		tcpcb_remove(n);
		return;
	}
	
	cb->remaddr = iptcp->iph->saddr;
	cb->remport = ntohs(h->sport);
	cb->irs = cb->seg_seq;
	cb->rcv_nxt = cb->irs + 1;
	cb->rcv_wnd = ntohs(h->window);
	
	cb->iss = choose_seq();
	cb->send_nxt = cb->iss;
	
	cb->state = TS_SYN_RECEIVED;
	cb->flags = TF_SYN|TF_ACK;
	

	cb->datalen = 0;	
	tcp_output(cb);
	
	cb->send_nxt = cb->iss + 1;
	cb->send_una = cb->iss;	
}

/*
 * This function is called from other states also to do
 * the standar processing. The only thing to matter is
 * that the state that calls it, must process and remove
 * the FIN flag. The only flag that causes a state change
 * here in this function
 */
void tcp_established(iptcp, cb)
struct iptcp_s *iptcp;
struct tcpcb_s *cb;	
{
	struct tcphdr_s *h;
	__u32 acknum;
	__u16 datasize;
	__u8 *data;
	
	h = iptcp->tcph;
		
	cb->rcv_wnd = ntohs(h->window);	
	
	datasize = iptcp->tcplen - TCP_DATAOFF(h);
	
	if(datasize != 0){
		/* Process the data */
		data = (__u8 *)h + TCP_DATAOFF(h);
		/* FIXME : check if it fits */
		if(datasize > CB_BUF_SPACE(cb))return;
		tcpcb_buf_write(cb, data, datasize);	
	
		if(h->flags & TF_PSH || CB_BUF_SPACE(cb) == 0){
			if(cb->bytes_to_push <=0)tcpcb_need_push++;
			cb->bytes_to_push = CB_BUF_USED(cb);
		}
		tcpdev_checkread(cb);
	}
	
	if(h->flags & TF_ACK){
		acknum = ntohl(h->acknum);
		if(SEQ_LT(cb->send_una, acknum))
			cb->send_una = acknum;
	}
	if(h->flags & TF_RST){
		/* TODO: Check seqnum for security */
		rmv_all_retrans(cb);
		if(cb->state == TS_CLOSE_WAIT){
			ENTER_TIME_WAIT(cb);
		} else {
			cb->state = TS_CLOSED;
			tcpdev_sock_state(cb, SS_UNCONNECTED);
		}
		return;
	}
	if(h->flags & TF_FIN){
		cb->rcv_nxt ++;
		cb->state = TS_CLOSE_WAIT;
	}
	
	if(datasize == 0 && ((h->flags & TF_ALL) == TF_ACK))
		return;
		
	cb->rcv_nxt += datasize;
	cb->flags = TF_ACK;	
	cb->datalen = 0;
	tcp_output(cb);
	
}

void tcp_synrecv(iptcp, cb)
struct iptcp_s *iptcp;
struct tcpcb_s *cb;	
{
	struct tcphdr_s *h;
	
	h = iptcp->tcph;
	
	if(h->flags & TF_RST){
		cb->state = TS_LISTEN; /*FIXME not valid any more */
		return;
	}	
	
	if(!h->flags & TF_ACK){
		printf("NO ACK IN SYNRECV\n");
		return;
	}
	cb->state = TS_ESTABLISHED;
	tcpdev_checkaccept(cb);
	
	tcp_established(iptcp, cb);
}

void tcp_fin_wait_1(iptcp, cb)
struct iptcp_s *iptcp;
struct tcpcb_s *cb;
{
	__u32 lastack;
	int needack = 0;
	
	if(iptcp->tcph->flags & TF_FIN){
		cb->rcv_nxt ++;
		/* Remove the flag */
		iptcp->tcph->flags &= ~TF_FIN;
		cb->state = TS_CLOSING;
		needack = 1;
	}
	
	lastack = cb->send_una;
	/* Process like there was no FIN */
	tcp_established(iptcp, cb);
	
	if(SEQ_LT(lastack, cb->send_una)){
		/* out FIN was acked */
		cb->state = TS_FIN_WAIT_2;
	}
	
	if(needack){
		cb->flags = TF_ACK;	/* TODO : Unify this somewhere */
		cb->datalen = 0;
		tcp_output(cb);
	}
}

void tcp_fin_wait_2(iptcp, cb)
struct iptcp_s *iptcp;
struct tcpcb_s *cb;
{
	int needack = 0;
	
	if(iptcp->tcph->flags & TF_FIN){
		cb->rcv_nxt ++;
		/* Remove the flag */
		iptcp->tcph->flags &= ~TF_FIN;
		ENTER_TIME_WAIT(cb);
		needack = 1;
	}
	
	/* Process like there was no FIN */
	tcp_established(iptcp, cb);
	
	if(needack){
		cb->flags = TF_ACK;	/* TODO : Unify this somewhere */
		cb->datalen = 0;
		tcp_output(cb);
	}
}

void tcp_closing(iptcp, cb)
struct iptcp_s *iptcp;
struct tcpcb_s *cb;
{
	__u32 lastack;
	lastack = cb->send_una;
	if(iptcp->tcph->flags & TF_FIN){
		cb->rcv_nxt ++;
		/* Remove the flag */
		iptcp->tcph->flags &= ~TF_FIN;
	}

	/* Process like there was no FIN */
	tcp_established(iptcp, cb);
	
	if(SEQ_LT(lastack, cb->send_una)){
		/* out FIN was acked */
		ENTER_TIME_WAIT(cb);
	}
}

void tcp_last_ack(iptcp, cb)
struct iptcp_s *iptcp;
struct tcpcb_s *cb;
{
	__u32 lastack;
	lastack = cb->send_una;
	
	if(iptcp->tcph->flags & TF_FIN){
		cb->rcv_nxt ++;
		/* Remove the flag */
		iptcp->tcph->flags &= ~TF_FIN;
	}

	/* Process like there was no FIN */
	tcp_established(iptcp, cb);
	
	if(SEQ_LT(lastack, cb->send_una)){
		/* out FIN was acked */
		cb->state = TS_CLOSED;
		tcpcb_remove(cb); /* Remove the cb */
	}
	
}

void tcp_update()
{
	if(cbs_in_time_wait > 0){
		tcpcb_expire_time_wait();
	}
	if(tcpcb_need_push > 0){
		tcpcb_push_data();
	}
}

void tcp_process(iph)
struct iphdr_s *iph;
{
	struct iptcp_s	iptcp;
	struct tcphdr_s *tcph;
	struct tcpcb_list_s	*cbnode;
	struct tcpcb_s *cb;	
	__u16	tmp;
	
	tcph = (struct tcphdr_s *)(((char *)iph) + 4 * IP_IHL(iph));
	iptcp.iph = iph;
	iptcp.tcph = tcph;
	iptcp.tcplen = ntohs(iph->tot_len) - 4 * IP_IHL(iph);
	
	if(tcp_chksum(&iptcp) != 0){
		/* printf("TCP check sum failed\n"); */
		return;
	}
		
	cbnode = tcpcb_find(iph->saddr, ntohs(tcph->dport), ntohs(tcph->sport));

	if(!cbnode){
		/* printf("Refusing packet \n"); */
		/* TODO : send RST and stuff */
		return;
	}
	
	cb = &cbnode->tcpcb;
	
	if(cb->state != TS_LISTEN && cb->state != TS_SYN_SENT && cb->state != TS_SYN_RECEIVED){	
		if(cb->rcv_nxt != ntohl(tcph->seqnum)){
			if(cb->rcv_nxt != ntohl(tcph->seqnum + 1))tcp_send_ack(cb);
			return;
		}	
			/* for now
					 * TODO queue up datagramms not in
					 * order and process them in order
				 	*/
	}
	
	switch (cb->state) {
	case TS_LISTEN:
		tcp_listen(&iptcp, cb);
		break;
	case TS_SYN_SENT:
		tcp_syn_sent(&iptcp, cb);
		break;
	case TS_SYN_RECEIVED:
		tcp_synrecv(&iptcp, cb);
		break;
	case TS_ESTABLISHED:
	case TS_CLOSE_WAIT:
		tcp_established(&iptcp, cb);
		break;
	case TS_FIN_WAIT_1:
		tcp_fin_wait_1(&iptcp, cb);
		break;
	case TS_FIN_WAIT_2:
		tcp_fin_wait_2(&iptcp, cb);
		break;
	case TS_CLOSING:
		tcp_closing(&iptcp, cb);
		break;	
	}
	
}

