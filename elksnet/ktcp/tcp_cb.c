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

#include "config.h"
#include "tcp.h"
#include "tcpdev.h"

static struct tcpcb_list_s	*tcpcbs;
int cbs_in_time_wait, cbs_in_user_timeout;

void tcpcb_init(void)
{
    tcpcbs = NULL;
    tcpcb_need_push = 0;

#ifdef CONFIG_INET_STATUS
    tcpcb_num = 0;
#endif

    cbs_in_time_wait = 0;
    cbs_in_user_timeout = 0;
}

void tcpcb_printall(void)
{
#if 0
    struct tcpcb_list_s *n = tcpcbs;

    printf("--- Control Blocks --- %d (%d)\n",tcpcb_num,tcp_retrans_memory);
    while (n) {
	printf("CB:%p sock:0x%x 0x%x State:%d LP:%d RP:%d RTT: %d ms unacc : %d\n",&n->tcpcb, n->tcpcb.sock, 
		n->tcpcb.newsock, n->tcpcb.state, n->tcpcb.localport,
		n->tcpcb.remport, n->tcpcb.rtt * 1000 / 16,
		n->tcpcb.unaccepted);
	n = n->next;
    }
#endif
}

#ifdef CONFIG_INET_STATUS

struct tcpcb_s *tcpcb_getbynum(int num)
{
    struct tcpcb_list_s *n;
    int i;

    if (num >= tcpcb_num)
	return NULL;

    for (n=tcpcbs, i=0; ; n=n->next, i++)
	if (i == num)
	    return &n->tcpcb;
}

#endif

struct tcpcb_list_s *tcpcb_new(void)
{
    struct tcpcb_list_s *n;

    n = (struct tcpcb_list_s *) malloc(sizeof(struct tcpcb_list_s));
    if (n == NULL) {
	printf("ERROR : Out of memory\n");
	return NULL;
    }

    memset(&n->tcpcb, 0, sizeof(struct tcpcb_s));
    n->tcpcb.rtt = 4 << 4; /* 4 sec */

    /* Link it to the list */
    if (tcpcbs) {
	n->next = tcpcbs;
	n->prev = NULL;
	tcpcbs->prev = n;
	tcpcbs = n;
    } else
	tcpcbs = n;

#ifdef CONFIG_INET_STATUS	
    tcpcb_num++;
#endif

    return n;
}

struct tcpcb_list_s *tcpcb_clone(struct tcpcb_s *cb)
{
    struct tcpcb_list_s *n = tcpcb_new();

    if (n)
	memcpy(&n->tcpcb, cb, sizeof(struct tcpcb_s));

    return n;
}

void tcpcb_remove(struct tcpcb_list_s *n)
{
    struct tcpcb_list_s *next = n->next;

#ifdef CONFIG_INET_STATUS	
    tcpcb_num--;
#endif

    if (n->prev)
	n->prev->next = next;
    else {

	/* Head update */
	n = next;
	n->prev = NULL;

	rmv_all_retrans(tcpcbs);
	free(tcpcbs);
	tcpcbs = n;

	return;
    }
	
    if (next)
	next->prev = n->prev;
	
    rmv_all_retrans(n);
    free(n);
}

struct tcpcb_list_s *tcpcb_check_port(__u16 lport)
{
    struct tcpcb_list_s *n;

    for (n=tcpcbs; n; n=n->next)
	if (n->tcpcb.localport == lport)
	    return n;

    return NULL;
}

struct tcpcb_list_s *tcpcb_find(__u32 addr, __u16 lport, __u16 rport)
{
    struct tcpcb_list_s *n;

    for (n=tcpcbs; n; n=n->next)
	if (n->tcpcb.remaddr == addr && n->tcpcb.remport == rport
				     && n->tcpcb.localport == lport)
	    return n;

    for (n=tcpcbs; n; n=n->next)
	if (n->tcpcb.remport == 0 && n->tcpcb.localport == lport)
	    return n;

    return NULL;
}

struct tcpcb_list_s *tcpcb_find_by_sock(__u16 sock)
{
    struct tcpcb_list_s *n;

    for (n=tcpcbs; n; n=n->next)
	if (n->tcpcb.sock == sock && n->tcpcb.unaccepted == 0)
	    return n;

    return NULL;
}

struct tcpcb_list_s *tcpcb_find_unaccepted(__u16 sock)
{
    struct tcpcb_list_s *n;

    for (n=tcpcbs; n; n=n->next)
	if (n->tcpcb.sock == sock && n->tcpcb.unaccepted == 1)
	    return n;

    return NULL;
}

void tcpcb_rmv_all_unaccepted(struct tcpcb_s *cb)
{
    struct tcpcb_list_s *n = tcpcbs, *next;

    while (n) {
	next = n->next;
	if (n->tcpcb.sock == cb->sock && n->tcpcb.unaccepted)
	    tcpcb_remove(n);
	n = next;
    }
}

void tcpcb_expire_timeouts(void)
{
    struct tcpcb_list_s *n = tcpcbs, *next;

    while (n) {
	next = n->next;
	switch (n->tcpcb.state) {
	    case TS_TIME_WAIT:
		if (TIME_GT(Now, n->tcpcb.time_wait_exp)) {
		    LEAVE_TIME_WAIT(&n->tcpcb);
		    tcpcb_remove(n);
		}
		break;
	    case TS_FIN_WAIT_1:
	    case TS_FIN_WAIT_2:
	    case TS_LAST_ACK:
	    case TS_CLOSING:
		if (TIME_GT(Now - (240 << 4), n->tcpcb.time_wait_exp)) {
		    cbs_in_user_timeout--;
		    tcpcb_remove(n);
		}
		break;
	}
	n = next;
    }
}

void tcpcb_push_data(void)
{
    struct tcpcb_list_s *n;

    for (n=tcpcbs; n; n=n->next)
	if (n->tcpcb.bytes_to_push > 0)
	    tcpdev_checkread(&n->tcpcb);
}

/* There must be free space greater-equal than len */
int tcpcb_buf_write(struct tcpcb_s *cb, __u8 *data, __u16 len)
{
    int tail;
    register int i;

    tail = cb->buf_head + cb->buf_len;
    for (i=0; i<len; i++)
	cb->in_buf[tail++ & (CB_IN_BUF_SIZE - 1)] = *(data + i);
    cb->buf_len += len;
}

/* same here */
int tcpcb_buf_read(struct tcpcb_s *cb, __u8 *data, __u16 len)
{
    register int head = cb->buf_head, i;

    for (i=0; i<len; i++)
	*(data + i) = cb->in_buf[head++ & (CB_IN_BUF_SIZE - 1)];
    cb->buf_head = head;
    cb->buf_len -= len;
}
