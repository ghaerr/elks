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

#define __KERNEL__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linuxmt/tcpdev.h>
#include <linuxmt/net.h>
#include <linuxmt/in.h>
#include <linuxmt/errno.h>

#include "netorder.h"
#include "tcp.h"
#include "tcpdev.h"
#include "netconf.h"

#define TCPDEV_BUFSIZE	2046

static unsigned char sbuf[TCPDEV_BUFSIZE];

static int tcpdevfd;

extern int cbs_in_user_timeout;

int tcpdev_init(char *fdev)
{
    tcpdevfd = open(fdev, O_NONBLOCK | O_RDWR);
    if (tcpdevfd < 0)
	printf("ERROR : failed to open tcpdev device %s\n",fdev);
    return tcpdevfd;    
}

void retval_to_sock(__u16 sock,short int r)
{
    struct tdb_return_data *ret_data = sbuf;

    ret_data->type = 0;
    ret_data->ret_value = r;
    ret_data->sock = sock;
    write(tcpdevfd, sbuf, sizeof(struct tdb_return_data));
}

static void tcpdev_bind(void)
{
    struct tdb_bind *db = sbuf;
    struct tcpcb_list_s *n;
    __u16 port;

    if (db->addr.sin_family != AF_INET) {
	retval_to_sock(db->sock,-EINVAL);
	return;
    }
	
    n = tcpcb_new();
    if (n == NULL) {
	retval_to_sock(db->sock,-ENOMEM);
	return;
    }
		
    port = ntohs(db->addr.sin_port);
    if (port == 0) {
	if (next_port < 1024)
	    next_port = 1024;
	while (tcpcb_check_port(next_port) != NULL)
	    next_port++;
	port = next_port;

/* TODO : Handle the case then no port is available
 * Just to be right, I don't think it is posible to run out
 * of ports in 8086!!!
 */

    } else {
	if (tcpcb_check_port(port) != NULL) {	/* Port already bound */
	    tcpcb_remove(n);
	    retval_to_sock(db->sock,-EINVAL);
	    return;
	}
    }	

    n->tcpcb.sock = db->sock;
    n->tcpcb.localaddr = local_ip;
    n->tcpcb.localport = port;
    n->tcpcb.state = TS_CLOSED;

    retval_to_sock(db->sock,0);
}

static void tcpdev_accept(void)
{
    struct tcpcb_list_s *n,*newn;
    struct tdb_accept *db = sbuf;
    struct tdb_accept_ret *ret_data;
    struct tcpcb_s *cb;
    __u16  sock = db->sock;

    n = tcpcb_find_by_sock(sock);
    if (!n || n->tcpcb.state != TS_LISTEN) {
	retval_to_sock(db->sock,-EINVAL);
	return;
    }
    cb = &n->tcpcb;
    newn = tcpcb_find_unaccepted(sock);
    if (!newn) {
	if (db->nonblock)
	    retval_to_sock(db->sock,-EAGAIN);
	else
	    cb->newsock = db->newsock;
	return;
    }

    cb = &newn->tcpcb;
    cb->unaccepted = 0;

    cb->sock = db->newsock;
    cb->newsock = 0;

    ret_data = sbuf;
    ret_data->type = 0;
    ret_data->ret_value = 0;
    ret_data->sock = sock;
    ret_data->addr_ip = cb->remaddr;
    ret_data->addr_port = cb->remport;

    write(tcpdevfd, sbuf, sizeof(struct tdb_accept_ret));
}

void tcpdev_checkaccept(struct tcpcb_s *cb)
{
    struct tdb_accept_ret *ret_data;
    struct tcpcb_s *listencb;

    if (!cb->unaccepted)
	return;
    listencb = &tcpcb_find_by_sock(cb->newsock)->tcpcb;

    ret_data = sbuf;
    ret_data->type = 123;
    ret_data->ret_value = 0;
    ret_data->sock = cb->sock;
    ret_data->addr_ip = cb->remaddr;
    ret_data->addr_port = cb->remport;

    cb->sock = listencb->newsock;
    cb->unaccepted = 0;
    listencb->newsock = 0;
    write(tcpdevfd, sbuf, sizeof(struct tdb_accept_ret));
}

static void tcpdev_connect(void)
{
    struct tdb_connect *db = sbuf;
    struct tcpcb_list_s *n;	

    n = tcpcb_find_by_sock(db->sock);
    if (!n || n->tcpcb.state != TS_CLOSED) {
	printf("KTCP Panic in connect\n");
	exit(1);
    }

    n->tcpcb.remaddr = db->addr.sin_addr.s_addr;
    n->tcpcb.remport = ntohs(db->addr.sin_port);

    if (n->tcpcb.remport == NETCONF_PORT && n->tcpcb.remaddr == 0) {
	n->tcpcb.state = TS_ESTABLISHED;
	retval_to_sock(n->tcpcb.sock, 0);
    } else
	tcp_connect(&n->tcpcb);
}

static void tcpdev_listen(void)
{
    struct tdb_listen *db = sbuf;
    struct tcpcb_list_s *n;	

    n = tcpcb_find_by_sock(db->sock);
    if (!n || n->tcpcb.state != TS_CLOSED) {
	retval_to_sock(db->sock,-EINVAL);
	return;
    }
    n->tcpcb.state = TS_LISTEN;
    n->tcpcb.newsock = 0;
    retval_to_sock(db->sock, 0);
}

void tcpdev_read(void)
{
    struct tdb_read *db = sbuf;
    struct tcpcb_list_s *n;
    struct tcpcb_s *cb;
    struct tdb_return_data *ret_data;
    int data_avail;
    register int bcc_bug;
    __u16 sock = db->sock;

    n = tcpcb_find_by_sock(sock);
    if (!n) {
	printf("KTCP Panic in read\n");
	exit(1);
    }

    cb = &n->tcpcb;
    if (cb->state == TS_CLOSING && cb->state == TS_LAST_ACK
				&& cb->state == TS_TIME_WAIT) {
	retval_to_sock(sock, -EPIPE);
	return;
    }

    if (cb->remport == NETCONF_PORT && cb->remaddr == 0)
	netconf_send(cb);	

    data_avail = cb->bytes_to_push;

    if (data_avail == 0) {
	if (cb->state == TS_CLOSE_WAIT)
	    retval_to_sock(sock, -EPIPE);
	else if (db->nonblock)
	    retval_to_sock(sock, -EAGAIN);
	else
	    cb->wait_data = db->size;
	return;
    }

    data_avail = db->size < data_avail ? db->size : data_avail;
    cb->bytes_to_push -= data_avail;
    if (cb->bytes_to_push <= 0)
	tcpcb_need_push--;

    ret_data = sbuf;
    ret_data->type = 0;
    ret_data->ret_value = data_avail;
    ret_data->sock = sock;
    tcpcb_buf_read(cb, &ret_data->data, data_avail);

    write(tcpdevfd, sbuf, sizeof(struct tdb_return_data) + data_avail - 1);		
}

void tcpdev_checkread(struct tcpcb_s *cb)
{
    struct tdb_return_data *ret_data = sbuf;
    int data_avail = CB_BUF_USED(cb);
    __u16 sock;

    if (cb->bytes_to_push <= 0)
	return;

    if (cb->wait_data == 0) {

	/* Update the avail_data in the kernel socket (for select) */
	sock = cb->sock;
	ret_data->type = TDT_AVAIL_DATA;
	ret_data->ret_value = cb->bytes_to_push;
	ret_data->sock = sock;

	write(tcpdevfd, sbuf, sizeof(struct tdb_return_data));
	return;	
    }

    data_avail = cb->wait_data < cb->bytes_to_push ? cb->wait_data : cb->bytes_to_push;
    cb->bytes_to_push -= data_avail;
    if (cb->bytes_to_push <= 0)
	tcpcb_need_push--;

    ret_data->type = 0;
    ret_data->ret_value = data_avail;
    ret_data->sock = cb->sock;
    tcpcb_buf_read(cb, &ret_data->data, data_avail);

    write(tcpdevfd, sbuf, sizeof(struct tdb_return_data) + data_avail - 1);

    cb->wait_data = 0;
}

static void tcpdev_write(void)
{
    struct tdb_write *db = sbuf;
    struct tcpcb_list_s *n;
    struct tcpcb_s *cb;
    struct tdb_return_data *ret_data;
    __u16  sock = db->sock;

    db = sbuf;
    sock = db->sock;

    /* This is a bit ugly but I'm to lazy right now */
    if (tcp_retrans_memory > TCP_RETRANS_MAXMEM) {
	retval_to_sock(sock, -ERESTARTSYS);
	return;
    }

    n = tcpcb_find_by_sock(sock);
    if (!n) {
	printf("KTCP panic in write (unknown sock:0x%x)\n",sock);
	return;
    }

    cb = &n->tcpcb;

    if (cb->state != TS_ESTABLISHED && cb->state != TS_CLOSE_WAIT) {
	retval_to_sock(sock, -EPIPE);/* FIXME : is this the right error? */
	return;
    }

    if (cb->remport == NETCONF_PORT && cb->remaddr == 0) {
	if (db->size == sizeof(struct stat_request_s))
	    netconf_request(db->data);
	retval_to_sock(sock, db->size);
	return;	
    }

    cb->flags = TF_PSH|TF_ACK;
    cb->datalen = db->size;
    cb->data = db->data;

    tcp_output(cb);

    retval_to_sock(sock, db->size);
    return;
}

static void tcpdev_release(void)
{
    struct tdb_release *db = sbuf;
    struct tcpcb_list_s *n;
    struct tcpcb_s *cb;
    __u16 sock = db->sock;

    n = tcpcb_find_by_sock(sock);
    if (n) {
	cb = &n->tcpcb;
	switch(cb->state){
	    case TS_CLOSED:
		tcpcb_remove(n);
		break;
	    case TS_LISTEN:
	    case TS_SYN_SENT:
		tcpcb_rmv_all_unaccepted(&n->tcpcb);/* FIXME : Properly disconnect all this */
		tcpcb_remove(n);
		break;
	    case TS_SYN_RECEIVED:
	    case TS_ESTABLISHED:
		if (cb->remport == NETCONF_PORT && cb->remaddr == 0){		
			tcpcb_remove(n);
			return;	
		}
		cb->state = TS_FIN_WAIT_1;
		cbs_in_user_timeout++;
		cb->time_wait_exp = Now;
		
		tcp_send_fin(cb);
		/* Handle it as abort */
#if 0
		tcp_send_reset(cb);
		tcpcb_remove(cb);
#endif
		break;
	    case TS_CLOSE_WAIT:
		cb->state = TS_LAST_ACK;
		cbs_in_user_timeout++;
		cb->time_wait_exp = Now;
		tcp_send_fin(cb);
		break;
	}
    }
}

void tcpdev_sock_state(struct tcpcb_s *cb, int state)
{
    struct tdb_return_data *ret_data = sbuf;
    __u16 sock = cb->sock;

    ret_data->type = TDT_CHG_STATE;
    ret_data->ret_value = state;
    ret_data->sock = sock;

    write(tcpdevfd, sbuf, sizeof(struct tdb_return_data));
}

void tcpdev_process(void)
{
    struct tdb_bind *db;
    int len = read(tcpdevfd, sbuf, TCPDEV_BUFSIZE);

    if (len <= 0)
	return;

#ifdef DEBUG
    printf("tcpdev_process : read %d bytes\n",len);
#endif

    switch (sbuf[0]){
	case TDC_BIND:
	    tcpdev_bind();
	    break;
	case TDC_ACCEPT:
	    tcpdev_accept();
	    break;
	case TDC_CONNECT:
	    tcpdev_connect();
	    break;
	case TDC_LISTEN:
	    tcpdev_listen();
	    break;
	case TDC_RELEASE:
	    tcpdev_release();
	    break;
	case TDC_READ:
	    tcpdev_read();
	    break;
	case TDC_WRITE:
	    tcpdev_write();
	    break;
    }
}
