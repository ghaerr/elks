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

static struct	tcpcb_list_s	*tcpcbs;
int cbs_in_time_wait;

void tcpcb_init()
{
	tcpcbs = NULL;
#ifdef CONFIG_INET_STATUS
	tcpcb_num = 0;
#endif
	cbs_in_time_wait = 0;
}

void tcpcb_printall()
{
	struct tcpcb_list_s *n;
	
	n = tcpcbs;
	printf("--- Control Blocks --- %d (%d)\n",tcp_timeruse,tcp_retrans_memory);
	while(n){
		printf("CtrlBlock:%p State:%d LPort:%d RPort:%d RTT: %d ms\n",&n->tcpcb, n->tcpcb.state, 
				n->tcpcb.localport, n->tcpcb.remport,n->tcpcb.rtt * 1000 / 16);
		n = n->next;
	}
}

#ifdef CONFIG_INET_STATUS
struct tcpcb_s *tcpcb_getbynum(num)
int num;
{
	struct tcpcb_list_s *n;
	int i;
	
	if(num >= tcpcb_num)return NULL;
	 	
	for(n = tcpcbs, i = 0 ;; n = n->next,i++){		
		if(i == num)return &n->tcpcb;
	}
}
#endif

struct tcpcb_list_s *
tcpcb_new()
{
	struct tcpcb_list_s *n;
	
	n = (struct tcpcb_list_s *)malloc(sizeof(struct tcpcb_list_s));
	if(n == NULL){
		printf("ERROR : Out of memory\n");
		return NULL;
	}
	
	memset(&n->tcpcb, 0, sizeof(struct tcpcb_s));
	n->tcpcb.rtt = 4 << 4; /* 3 sec */
	
	/* Link it to the list */
	if(tcpcbs){
		n->next = tcpcbs;
		n->prev = NULL;
		tcpcbs->prev = n;
		tcpcbs = n;
	}
	else {
		tcpcbs = n;
	}
#ifdef CONFIG_INET_STATUS	
	tcpcb_num++;
#endif
	return n;
}

struct tcpcb_list_s *tcpcb_clone(cb)
struct tcpcb_s *cb;
{
	struct tcpcb_list_s *n;
	
	n = tcpcb_new();
	if(n == NULL)
		return NULL;
		
	memcpy(&n->tcpcb, cb, sizeof(struct tcpcb_s));
	
	return n;
}


void tcpcb_remove(n)
struct tcpcb_list_s *n;
{
	struct tcpcb_list_s *next;
#ifdef CONFIG_INET_STATUS	
	tcpcb_num--;
#endif	
	next = n->next;
	
	if(n->prev){
		n->prev->next = next;
	} else {
		/* Head update */
		n = next;
		n->prev = NULL;
		
		rmv_all_retrans(tcpcbs);
		free(tcpcbs);
		tcpcbs = n;
	
		return;
	}
	
	if(next){
		next->prev = n->prev;
	}
	
	rmv_all_retrans(n);	
	free(n);
	n = NULL;
}

struct tcpcb_list_s *
tcpcb_check_port(lport)
__u16 lport;
{
	struct tcpcb_list_s *n;
	
	for(n = tcpcbs ; n != NULL ; n = n->next){
		if(n->tcpcb.localport == lport)
			return n;
	}	
	return NULL;
}

struct tcpcb_list_s *
tcpcb_find(addr, lport, rport)
__u32 addr;
__u16 lport;
__u16 rport;
{
	struct tcpcb_list_s *n;
	
	for(n = tcpcbs ; n != NULL ; n = n->next){
		if(n->tcpcb.remaddr == addr && n->tcpcb.remport == rport &&
			n->tcpcb.localport == lport){
			
			return n;
		}
	}
	
	for(n = tcpcbs ; n != NULL ; n = n->next){
		if(n->tcpcb.remport == 0 &&
			n->tcpcb.localport == lport){
			
			return n;
		}
	}
	return NULL;
}

struct tcpcb_list_s *
tcpcb_find_by_sock(sock)
__u16 sock;
{
	struct tcpcb_list_s *n;
	
	for(n = tcpcbs ; n != NULL ; n = n->next){
		if(n->tcpcb.sock == sock && n->tcpcb.unaccepted == 0){
			return n;
		}
	}
	
	return NULL;
}

struct tcpcb_list_s *
tcpcb_find_unaccepted(sock)
__u16 sock;
{
	struct tcpcb_list_s *n;
	
	for(n = tcpcbs ; n != NULL ; n = n->next){
		if(n->tcpcb.sock == sock && n->tcpcb.unaccepted == 1){
			return n;
		}
	}
	
	return NULL;
}


void tcpcb_rmv_all_unaccepted(cb)
struct tcpcb_s *cb;
{
	struct tcpcb_list_s *n,*next;
	
	n = tcpcbs;
	while(n){
		next = n->next;
		if(n->tcpcb.sock == cb->sock && n->tcpcb.unaccepted){
			tcpcb_remove(n);
		}
		n = next;
	}
}

void tcpcb_expire_time_wait()
{
	struct tcpcb_list_s *n,*next;
	
	n = tcpcbs;
	while(n){
		next = n->next;
		if(n->tcpcb.state == TS_TIME_WAIT && TIME_GT(n->tcpcb.time_wait_exp, Now)){
			LEAVE_TIME_WAIT(&n->tcpcb);
			tcpcb_remove(n);
		}
		n = next;
	}
}


/* There must be free space greater-equal than len */
int tcpcb_buf_write(cb, data, len)
struct tcpcb_s	*cb;
__u8 *data;
__u16 len;
{
	int len1 = 0;

	if(CB_IN_BUF_SIZE <= cb->buf_head + len){
		len1 = CB_IN_BUF_SIZE - cb->buf_head;
		memcpy(cb->in_buf + cb->buf_head, data, len1);
		cb->buf_head = 0;
	}

	len = len - len1;
	memcpy(cb->in_buf + cb->buf_head, data, len);
	cb->buf_head += len;
	
}

/* same here */
int tcpcb_buf_read(cb, data, len)
struct tcpcb_s	*cb;
__u8 *data;
__u16 len;
{
	int len1 = 0;
	
	if(CB_IN_BUF_SIZE <= cb->buf_tail + len){
		len1 = CB_IN_BUF_SIZE - cb->buf_tail;
		memcpy(data, cb->in_buf + cb->buf_tail, len1);
		cb->buf_tail = 0;
	}

	len = len - len1;
	memcpy(data, cb->in_buf + cb->buf_tail, len);
	cb->buf_tail += len;
}
