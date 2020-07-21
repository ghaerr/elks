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
#include <stdlib.h>
#include <string.h>

#include "slip.h"
#include "ip.h"
#include "tcp.h"
#include <linuxmt/arpa/inet.h>
#include "timer.h"
#include "tcpdev.h"
#include "netconf.h"

static struct tcp_retrans_list_s *retrans_list;
static unsigned char tcpbuf[TCP_BUFSIZ];

__u16 tcp_chksum(struct iptcp_s *h)
{
    __u32 sum = htons(h->tcplen);
    __u16 len = h->tcplen;
    register __u16 *data = (__u16 *)h->tcph;

    for (; len > 1 ; len -= 2)
	sum += *data++;

    if (len == 1)
	sum += (__u16)(*(__u8 *)data);

    sum += h->iph->saddr & 0xffff;
    sum += (h->iph->saddr >> 16) & 0xffff;
    sum += h->iph->daddr & 0xffff;
    sum += (h->iph->daddr >> 16) & 0xffff;
    sum += htons((__u16)6);

    while (sum >> 16)
	sum = (sum & 0xffff) + (sum >> 16);
    return ~(__u16)sum;
}
/*** __u16 tcp_chksum(struct iptcp_s *h)
	.text
	.globl _tcp_chksum
_tcp_chksum:
	push	bp
	mov	bp, sp
	push	di
	push	si

	mov	bx, 4[bp]	! h
	mov	ax, 4[bx]	! h->tcplen
	mov	cx, ax
	xchg	al, ah

	mov	di, [bx]	! h->iph

	mov	si, 2[bx]	! h->tcph
	mov	bx, cx
	sar	cx, #$1

	mov	dx, bx
	and	bx, #$1
	jz	loop1

	mov	bx, dx
	dec	bx
	mov	dl, [si + bx]
	xor	dh, dh
	add	ax, dx

loop1:
	adc	ax, [si]
        inc si
        inc si
        loop	loop1

	adc	ax, $E[di]	! h->iph->saddr
	adc	ax, $C[di]
	adc	ax, $12[di]	! h->iph->daddr
	adc	ax, $10[di]
	adc	ax, #$600
	adc	ax, 0

        not	ax

	pop	si
	pop	di
	pop	bp
	ret
***/

__u16 tcp_chksumraw(struct tcphdr_s *h, __u32 saddr, __u32 daddr, __u16 len)
{
    __u32 sum = htons(len);
    __u16 *data = (__u16 *) h;

    for (; len > 1 ; len -= 2)
	sum += *data++;

    if (len == 1)
	sum += (__u16)(*(__u8 *) data);

    sum += saddr & 0xffff;
    sum += (saddr >> 16) & 0xffff;
    sum += daddr & 0xffff;
    sum += (daddr >> 16) & 0xffff;
    sum += htons((__u16)6);

    while (sum >> 16)
	sum = (sum & 0xffff) + (sum >> 16);
    return ~(__u16)sum;
}
/*** __u16 tcp_chksumraw(struct tcphdr_s *h, __u32 saddr, __u32 daddr, __u16 len)
	.text
	.globl _tcp_chksumraw
_tcp_chksumraw:
	push	bp
	mov	bp, sp
	push	di
	push	si

	mov	si, 4[bp]	! h
	mov	ax, $E[bp]	! len
	mov	cx, ax
	xchg	al, ah
	mov	bx, cx
	sar	cx, #$1

	mov	dx, bx
	and	bx, #$1
	jz	loop2

	mov	bx, dx
	dec	bx
	mov	dl, [si + bx]
	xor	dh, dh
	add	ax, dx

loop2:
	adc	ax, [si]
        inc si
        inc si
        loop	loop2

	adc	ax, 6[bp]
	adc	ax, 8[bp]
	adc	ax, $A[bp]
	adc	ax, $C[bp]
	adc	ax, #$600
        adc     ax, 0

        not	ax

	pop	si
	pop	di
	pop	bp
	ret
***/

struct tcp_retrans_list_s *
rmv_from_retrans(struct tcp_retrans_list_s *n)
{
    struct tcp_retrans_list_s *next = n->next;

    tcp_timeruse--;
    tcp_retrans_memory -= n->len;
    if (n->prev)
	n->prev->next = next;
    else {

	/* Head update */
	n = next;
	n->prev = NULL;
debug_mem("retrans free buffers %d mem %d\n", tcp_timeruse, tcp_retrans_memory);
	free(retrans_list);
	retrans_list = n;

	return n;
    }

    if (next)
	next->prev = n->prev;

debug_mem("retrans free buffers %d mem %d\n", tcp_timeruse, tcp_retrans_memory);
    free(n);

    return next;
}

void rmv_all_retrans(struct tcpcb_list_s *lcb)
{
    struct tcp_retrans_list_s *n;

    n = retrans_list;

    while (n != NULL)
	if (n->cb == &lcb->tcpcb)
	    n = rmv_from_retrans(n);
	else
	    n = n->next;
}

void rmv_all_retrans_cb(struct tcpcb_s *cb)
{
    struct tcp_retrans_list_s *n;

    n = retrans_list;

    while (n != NULL)
	if (n->cb == cb)
	    n = rmv_from_retrans(n);
	else
	    n = n->next;
}

void add_for_retrans(struct tcpcb_s *cb, struct tcphdr_s *th, __u16 len,
		     struct addr_pair *apair)
{
    struct tcp_retrans_list_s *n;

    if ((cb->flags & TF_ALL) == TF_ACK && cb->datalen == 0)
	return;

    if (cb->flags & TF_RST)
	return;

    if (cb->state == TS_CLOSED)	//FIXME remove
	return;

    n = (struct tcp_retrans_list_s *)malloc(sizeof(struct tcp_retrans_list_s) + len);
    if (n == NULL) {
	printf("ktcp: Out of memory\n");
	return;
    }

    n->cb = cb;

    /* Link it to the list */
    if (retrans_list) {
	n->next = retrans_list;
	n->prev = NULL;
	retrans_list->prev = n;
	retrans_list = n;
    } else {
	retrans_list = n;
	n->prev = n->next = NULL;
    }

    /* start timeout blocking in main loop*/
    tcp_timeruse++;

    tcp_retrans_memory += len;
debug_mem("retrans alloc buffers %d, mem %d\n", tcp_timeruse, tcp_retrans_memory);
    n->len = len;
    memcpy(n->tcphdr, th, len);
    memcpy(&n->apair, apair, sizeof(struct addr_pair));
    n->retrans_num = 0;
    n->first_trans = Now;

    n->rto = cb->rtt << 1;
    if (linkprotocol != LINK_ETHER && n->rto < TIMEOUT_MIN_SLIP)
	n->rto = TIMEOUT_MIN_SLIP;		/* 1/2 sec min retrans timeout for slip/cslip*/
    n->next_retrans = Now + n->rto;
}

void tcp_reoutput(struct tcp_retrans_list_s *n)
{
    n->retrans_num ++;
    n->rto *= 2;
    n->next_retrans = Now + n->rto;
printf("retrans retry #%d rto %ld mem %u\n", n->retrans_num, n->rto, tcp_retrans_memory);
    ip_sendpacket((unsigned char *)n->tcphdr, n->len, &n->apair);
    netstats.tcpretranscnt++;
}

/* called every ktcp cycle when tcp_timeruse nonzero*/
void tcp_retrans(void)
{
    struct tcp_retrans_list_s *n;
    int datalen, rtt;

/* FIXME avoid running out of memory - bug in retrans recovery*/
if (tcp_retrans_memory > TCP_RETRANS_MAXMEM /*|| tcp_timeruse > 5*/) {
	struct tcpcb_s *cb = NULL;
	printf("tcp: RETRANS limit, timeruse %d\n", tcp_timeruse);
	n = retrans_list;
	while (n != NULL) {
		if (!cb)
			cb = n->cb;
		n = rmv_from_retrans(n);
	}
	tcp_send_reset(cb);
	return;
}

    debug_mem("retrans check buffers %d, mem %d\n", tcp_timeruse, tcp_retrans_memory);

    n = retrans_list;
    while (n != NULL) {
	datalen = n->len - TCP_DATAOFF(&n->tcphdr[0]);

	//debug_tcp("retrans: %lx %lx", ntohl(n->tcphdr[0].seqnum) + datalen,n->cb->send_una);
	if (SEQ_LEQ(ntohl(n->tcphdr[0].seqnum) + datalen, n->cb->send_una)) {
	    if (n->retrans_num == 0) {
		rtt = Now - n->first_trans;
		if (rtt > 0)
		    n->cb->rtt = (TCP_RTT_ALPHA * n->cb->rtt + (100 - TCP_RTT_ALPHA) * rtt) / 100;
		debug_tcp("rtt %d RTT %ld RTO %ld\n", rtt, n->cb->rtt, n->rto*2);
	    }
	    n = rmv_from_retrans(n);
	    continue;
	}

//debug_tcp("retrans %d mem %d\n", tcp_timeruse, tcp_retrans_memory);
	if (TIME_GEQ(Now, n->next_retrans)) {
	    tcp_reoutput(n);
	    return;
	}
	n = n->next;
    }
}

void tcp_output(struct tcpcb_s *cb)
{
    struct tcphdr_s *th = (struct tcphdr_s *)tcpbuf;
    struct addr_pair apair;
    int header_len, len;

    th->sport = htons(cb->localport);
    th->dport = htons(cb->remport);
    th->seqnum = htonl(cb->send_nxt);
    th->acknum = htonl(cb->rcv_nxt);

    cb->send_nxt += cb->datalen;

    len = CB_BUF_SPACE(cb) - PUSH_THRESHOLD;
    if (len <= 0)
	len = 1;		/* Never advertise zero window size */
len = 255;	//FIXME testing only
    th->window = htons(len);
    th->urgpnt = 0;
    th->flags = cb->flags;

    header_len = sizeof(tcphdr_t);
    if (cb->flags & TF_SYN) {
	__u8 *options = th->options;

	options[0] = TCP_OPT_MSS;
	options[1] = TCP_OPT_MSS_LEN;		/* total options length (4)*/
	*(__u16 *)(options + 2) = htons(MTU - 40);
	//*(__u16 *)(options + 2) = htons(512 - 40);
	header_len += TCP_OPT_MSS_LEN;
    }

    TCP_SETHDRSIZE(th, header_len);

    if (cb->datalen)
	memcpy((char *)th + header_len, cb->data, cb->datalen);
    len = cb->datalen + header_len;

    th->chksum = 0;
    th->chksum = tcp_chksumraw(th, cb->localaddr, cb->remaddr, len);

    apair.saddr = cb->localaddr;
    apair.daddr = cb->remaddr;
    apair.protocol = PROTO_TCP;

    add_for_retrans(cb, th, len, &apair);
    ip_sendpacket((unsigned char *)th, len, &apair);
    netstats.tcpsndcnt++;
}
