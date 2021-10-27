/* This file is part of the ELKS TCP/IP stack
 *
 * (C) 2001 Harry Kalogirou (harkal@rainbow.cs.unipi.gr)
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include "slip.h"
#include "ip.h"
#include "icmp.h"
#include "tcp.h"
#include "tcp_cb.h"
#include "tcpdev.h"
#include "tcp_output.h"
#include "timer.h"
#include "netconf.h"

timeq_t Now;

extern int cbs_in_time_wait;		// FIXME remove extern
extern int cbs_in_user_timeout;

void tcp_print(struct iptcp_s *head, int recv, struct tcpcb_s *cb)
{
#if DEBUG_TCP
    debug_tcp("TCP: %s ", recv? "recv": "send");
    debug_tcp("src:%u dst:%u ", ntohs(head->tcph->sport), ntohs(head->tcph->dport));
    debug_tcp("flags:0x%x ",head->tcph->flags);
    if (cb) {
      if (recv) {
    	if (head->tcplen > 20) debug_tcp("seq:%ld-%ld ", ntohl(head->tcph->seqnum) - cb->irs, 
				ntohl(head->tcph->seqnum) - cb->irs+head->tcplen-20);
	debug_tcp("ack:%ld ", ntohl(head->tcph->acknum) - cb->iss);
      } else {
    	if (head->tcplen > 20) debug_tcp("seq:%ld-%ld ", ntohl(head->tcph->seqnum) - cb->iss, 
	ntohl(head->tcph->seqnum) - cb->iss+head->tcplen-20);
 	debug_tcp("ack:%ld ", ntohl(head->tcph->acknum) - cb->irs);
      }
    }
    debug_tcp("win:%u urg:%d ", ntohs(head->tcph->window), head->tcph->urgpnt);
    debug_tcp("chk:0x%x len:%u\n", tcp_chksum(head), head->tcplen);
#endif
}

int tcp_init(void)
{
    tcpcb_init();
    cbs_in_time_wait = 0;
    tcp_retrans_memory = 0;

    return 0;
}

static __u32 choose_seq(void)
{
    return timer_get_time();
}

void tcp_send_reset(struct tcpcb_s *cb)
{
    cb->flags = TF_RST;
    cb->datalen = 0;
    tcp_output(cb);
}

/* abruptly terminate connection*/
void tcp_reset_connection(struct tcpcb_s *cb)
{
    tcp_send_reset(cb);
    tcpcb_remove_cb(cb);	/* deallocate*/
}

void tcp_send_fin(struct tcpcb_s *cb)
{
    cb->flags = TF_FIN|TF_ACK;
    cb->datalen = 0;
    tcp_output(cb);
    cb->send_nxt++;
}

static void tcp_send_ack(struct tcpcb_s *cb)
{
    cb->flags = TF_ACK;
    cb->datalen = 0;
    tcp_output(cb);
}

/* entry point, not called from state machine*/
void tcp_connect(struct tcpcb_s *cb)
{
    cb->iss = choose_seq();
    cb->send_nxt = cb->iss;
    cb->send_una = cb->iss;

    cb->state = TS_SYN_SENT;
    cb->flags = TF_SYN;

    cb->datalen = 0;
    tcp_output(cb);
}

static void tcp_syn_sent(struct iptcp_s *iptcp, struct tcpcb_s *cb)
{
    struct tcphdr_s *h = iptcp->tcph;

    cb->seg_seq = ntohl(h->seqnum);
    cb->seg_ack = ntohl(h->acknum);

    if (h->flags & TF_RST) {
	retval_to_sock(cb->sock, -ECONNREFUSED);
	//cb->state = TS_CLOSED;
	tcpcb_remove_cb(cb); 	/* deallocate*/
	return;
    }

    if (h->flags & (TF_SYN|TF_ACK)) {
	if (cb->seg_ack != cb->send_una + 1) {
printf("SYN sent, wrong ACK (listen port not expired)\n");
	    /* Send RST */
	    cb->send_nxt = h->acknum;
	    //tcp_send_reset(cb);
	    retval_to_sock(cb->sock, -ECONNREFUSED);
	    tcpcb_remove_cb(cb);	/* deallocate*/
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

static void tcp_listen(struct iptcp_s *iptcp, struct tcpcb_s *lcb)
{
    struct tcpcb_list_s *n;
    struct tcpcb_s *cb;
    struct tcphdr_s *h = iptcp->tcph;

    if (h->flags & TF_RST)
	return;

    if (!(h->flags & TF_SYN)) {
	debug_tcp("tcp: no SYN in listen\n");
	return;
    }

    if (h->flags & TF_ACK)
	debug_tcp("tcp: ACK in listen not implemented\n");

    n = tcpcb_clone(lcb);    /* copy lcb into linked list*/
    if (!n)
	return;		     /* no memory for new connection*/
    cb = &n->tcpcb;
    cb->unaccepted = 1;      /* Mark as unaccepted */
    cb->newsock = lcb->sock; /* lcb-> is the socket in kernel space */

    cb->seg_seq = ntohl(h->seqnum);
    cb->seg_ack = ntohl(h->acknum);

    cb->remaddr = iptcp->iph->saddr; /* sender's ip address*/
    cb->remport = ntohs(h->sport);   /* sender's port*/
    cb->irs = cb->seg_seq;           /* sender's sequence number*/
    cb->rcv_nxt = cb->irs + 1;       /* ktcp's acknum */
    cb->rcv_wnd = ntohs(h->window);

    cb->iss = choose_seq();	/* our arbitrary sequence number*/
    cb->send_nxt = cb->iss;

    cb->state = TS_SYN_RECEIVED;
    cb->flags = TF_SYN|TF_ACK;

    cb->datalen = 0;
    tcp_output(cb);

    cb->send_nxt = cb->iss + 1;
    cb->send_una = cb->iss;	/* unack'd seqno*/
}

/*
 * This function is called from other states also to do the standard
 * processing. The only thing to matter is that the state that calls it,
 * must process and remove the FIN flag. The only flag that causes a
 * state change here in this function
 */

static void tcp_established(struct iptcp_s *iptcp, struct tcpcb_s *cb)
{
    struct tcphdr_s *h;
    __u32 acknum;
    __u16 datasize;
    __u8 *data;

    h = iptcp->tcph;

    cb->rcv_wnd = ntohs(h->window);

    datasize = iptcp->tcplen - TCP_DATAOFF(h);

    if (datasize != 0) {
	//debug_tcp("tcp: recv data len %u avail %u\n", datasize, CB_BUF_SPACE(cb));
	/* Process the data */
	data = (__u8 *)h + TCP_DATAOFF(h);

	/* check if buffer space for received packet*/
	if (datasize > CB_BUF_SPACE(cb)) {
	    printf("tcp: dropping packet, no buffer space: %u > %d\n", datasize, CB_BUF_SPACE(cb));
	    netstats.tcpdropcnt++;
	    return;
	}

	tcpcb_buf_write(cb, data, datasize);

	if (1 || (h->flags & TF_PSH) || CB_BUF_SPACE(cb) <= PUSH_THRESHOLD) {
	    //if (cb->bytes_to_push <= 0)
	    if (cb->bytes_to_push >= 0)
		tcpcb_need_push++;
	    cb->bytes_to_push = CB_BUF_USED(cb);
	}
	tcpdev_checkread(cb);
    }

    if (h->flags & TF_ACK) {		/* update unacked*/
	acknum = ntohl(h->acknum);
	if (SEQ_LT(cb->send_una, acknum))
	    cb->send_una = acknum;
    }

    if (h->flags & TF_RST) {
	/* TODO: Check seqnum for security */
	printf("tcp: RST from %s:%u->%d\n", in_ntoa(cb->remaddr), ntohs(h->sport), ntohs(h->dport));
	rmv_all_retrans_cb(cb);
	if (cb->state == TS_CLOSE_WAIT) {
	    ENTER_TIME_WAIT(cb);
	    tcpdev_sock_state(cb, SS_UNCONNECTED);	/* no wakeup?*/
	} else {
	    tcpdev_sock_state(cb, SS_DISCONNECTING);	/* wakes up process*/
	    tcpcb_remove_cb(cb); 	/* deallocate*/
	}
	return;
    }

    if (h->flags & TF_FIN) {
	cb->rcv_nxt++;
	cb->state = TS_CLOSE_WAIT;
	cb->time_wait_exp = Now;	/* used for debug output only*/
	tcpdev_sock_state(cb, SS_DISCONNECTING);
    }

    if (datasize == 0 && ((h->flags & TF_ALL) == TF_ACK))
	return; /* ACK with no data received - so don't answer*/

    cb->rcv_nxt += datasize;
    cb->flags = TF_ACK;
    cb->datalen = 0;
    tcp_output(cb);
}

static void tcp_synrecv(struct iptcp_s *iptcp, struct tcpcb_s *cb)
{
    struct tcphdr_s *h = iptcp->tcph;

    if (h->flags & TF_RST)
	cb->state = TS_LISTEN;		/* FIXME: not valid any more */
    else if ((h->flags & TF_ACK) == 0)
	debug_tcp("tcp: NO ACK IN SYNRECV\n");
    else {
	cb->state = TS_ESTABLISHED;
	tcpdev_checkaccept(cb);
	tcp_established(iptcp, cb);
    }
}

static void tcp_fin_wait_1(struct iptcp_s *iptcp, struct tcpcb_s *cb)
{
    __u32 lastack;
    int needack = 0;

    cb->time_wait_exp = Now;
    if (iptcp->tcph->flags & TF_FIN) {
	cb->rcv_nxt ++;

	/* Remove the flag */
	iptcp->tcph->flags &= ~TF_FIN;
	cb->state = TS_CLOSING; 	/* cbs_in_user_timeout stays unchanged */
	cb->time_wait_exp = Now;
	needack = 1;
    }

    lastack = cb->send_una;

    /* Process like there was no FIN */
    tcp_established(iptcp, cb);

    if (SEQ_LT(lastack, cb->send_una)) {

	/* out FIN was acked */
	cb->state = TS_FIN_WAIT_2;	/* cbs_in_user_timeout stays unchanged */
	cb->time_wait_exp = Now;
    }

    if (needack) {
	cb->flags = TF_ACK;		/* TODO : Unify this somewhere */
	cb->datalen = 0;
	tcp_output(cb);
    }
}

static void tcp_fin_wait_2(struct iptcp_s *iptcp, struct tcpcb_s *cb)
{
    int needack = 0;

    cb->time_wait_exp = Now;
    if (iptcp->tcph->flags & TF_FIN) {
	cb->rcv_nxt ++;

	/* Remove the flag */
	iptcp->tcph->flags &= ~TF_FIN;
	cbs_in_user_timeout--;
	ENTER_TIME_WAIT(cb);
	needack = 1;
    }

    /* Process like there was no FIN */
    tcp_established(iptcp, cb);

    if (needack) {
	cb->flags = TF_ACK;		/* TODO: Unify this somewhere */
	cb->datalen = 0;
	tcp_output(cb);
    }
}

static void tcp_closing(struct iptcp_s *iptcp, struct tcpcb_s *cb)
{
    __u32 lastack;

    cb->time_wait_exp = Now;
    lastack = cb->send_una;
    if (iptcp->tcph->flags & TF_FIN) {
	cb->rcv_nxt ++;

	/* Remove the flag */
	iptcp->tcph->flags &= ~TF_FIN;
    }

    /* Process like there was no FIN */
    tcp_established(iptcp, cb);

    if (SEQ_LT(lastack, cb->send_una)) {

	/* our FIN was acked */
	cbs_in_user_timeout--;
	ENTER_TIME_WAIT(cb);
    }
}

static void tcp_last_ack(struct iptcp_s *iptcp, struct tcpcb_s *cb)
{
    cb->time_wait_exp = Now;
    if (iptcp->tcph->flags & (TF_ACK|TF_RST)) {

	/* our FIN was acked */
	cbs_in_user_timeout--;
	cb->state = TS_CLOSED;
	tcpcb_remove_cb(cb); 	/* deallocate*/
    }
}

/* called every ktcp run cycle*/
void tcp_update(void)
{
    if (cbs_in_time_wait > 0 || cbs_in_user_timeout > 0) {
debug_tcp("ktcp: update %x,%x\n", cbs_in_time_wait, cbs_in_user_timeout);
	tcpcb_expire_timeouts();
    }

    //if (tcpcb_need_push > 0)
	tcpcb_push_data();
}

/* process an incoming TCP packet*/
void tcp_process(struct iphdr_s *iph)
{
    struct iptcp_s iptcp;
    struct tcphdr_s *tcph;
    struct tcpcb_list_s *cbnode;
    struct tcpcb_s *cb = NULL;

    tcph = (struct tcphdr_s *)(((char *)iph) + 4 * IP_HLEN(iph));
    iptcp.iph = iph;
    iptcp.tcph = tcph;
    iptcp.tcplen = ntohs(iph->tot_len) - 4 * IP_HLEN(iph);

    cbnode = tcpcb_find(iph->saddr, ntohs(tcph->dport), ntohs(tcph->sport));
    if (cbnode) cb = &cbnode->tcpcb;
    tcp_print(&iptcp, 1, cb);

    if (tcp_chksum(&iptcp) != 0) {
	printf("tcp: BAD CHECKSUM (0x%x) len %d\n", tcp_chksum(&iptcp), iptcp.tcplen);
	netstats.tcpbadchksum++;
	return;
    }

    if (!cbnode) {
	printf("tcp: refusing packet %s:%u->%d\n", in_ntoa(iph->saddr),
		ntohs(tcph->sport), ntohs(tcph->dport));

	if (tcph->flags & TF_RST)
		return;

	/* Dummy up a new control block and send RST to shutdown sender */
	cbnode = tcpcb_new();
	if (cbnode) {
	    __u32 seqno = ntohl(tcph->seqnum);
	    cbnode->tcpcb.state = TS_CLOSED;
	    cbnode->tcpcb.localaddr = iph->daddr;
	    cbnode->tcpcb.localport = ntohs(tcph->dport);
	    cbnode->tcpcb.remaddr = iph->saddr;
	    cbnode->tcpcb.remport = ntohs(tcph->sport);
	    if (tcph->flags & TF_ACK) {
		cbnode->tcpcb.flags = TF_RST;
		cbnode->tcpcb.send_nxt = ntohl(tcph->acknum);
	    } else
		cbnode->tcpcb.flags = TF_RST|TF_ACK;
	    cbnode->tcpcb.rcv_nxt = (tcph->flags & TF_SYN)? seqno+1: seqno;
	    tcp_output(&cbnode->tcpcb);		/* send RST*/
	    tcpcb_remove_cb(&cbnode->tcpcb);	/* deallocate*/
	}

	netstats.tcpdropcnt++;
	return;
    }


    if (cb->state != TS_LISTEN && cb->state != TS_SYN_SENT
			       && cb->state != TS_SYN_RECEIVED) {
	if (cb->rcv_nxt != ntohl(tcph->seqnum)) {
	    if (cb->rcv_nxt != ntohl(tcph->seqnum + 1))
		tcp_send_ack(cb);
	    return;
	}

	/*
	 * TODO queue up datagramms not in
	 * order and process them in order
	 */
    }

    switch (cb->state) {

	case TS_LISTEN:
	    debug_tcp("TS_LISTEN\n");
	    tcp_listen(&iptcp, cb);
	    break;

	case TS_SYN_SENT:
	    debug_tcp("TS_SYN_SENT\n");
	    tcp_syn_sent(&iptcp, cb);
	    break;

	case TS_SYN_RECEIVED:
	    debug_tcp("TS_SYN_RECEIVED\n");
	    tcp_synrecv(&iptcp, cb);
	    break;

	case TS_ESTABLISHED:
	    debug_tcp("TS_ESTABLISHED\n");
	    tcp_established(&iptcp, cb);
	    break;

	case TS_CLOSE_WAIT:
	    debug_tcp("TS_CLOSE_WAIT\n");
	    tcp_established(&iptcp, cb);
	    break;

	case TS_FIN_WAIT_1:
	    debug_tcp("TS_FIN_WAIT_1\n");
	    tcp_fin_wait_1(&iptcp, cb);
	    break;

	case TS_FIN_WAIT_2:
	    debug_tcp("TS_FIN_WAIT_2\n");
	    tcp_fin_wait_2(&iptcp, cb);
	    break;

	case TS_LAST_ACK:
	    debug_tcp("TS_LAST_ACK\n");
	    tcp_last_ack(&iptcp, cb);
	    break;

	case TS_CLOSING:
	    debug_tcp("TS_CLOSING\n");
	    tcp_closing(&iptcp, cb);
	    break;
    }
}
