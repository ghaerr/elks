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
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include "ip.h"
#include "tcp.h"
#include "tcpdev.h"
#include "tcp_cb.h"
#include "netconf.h"

static __u16	next_port;
static unsigned char sbuf[TCPDEV_BUFSIZ];

int tcpdevfd;

int tcpdev_init(char *fdev)
{
    int fd  = open(fdev, O_NONBLOCK | O_RDWR);
    if (fd < 0)
	printf("ktcp: can't open tcpdev device %s\n",fdev);
    return fd;
}

void notify_sock(void *sock, int type, int value)
{
    struct tdb_return_data return_data;

    return_data.type = type;
    return_data.ret_value = value;
    return_data.sock = sock;
    return_data.size = 0;
    write(tcpdevfd, &return_data, sizeof(return_data));
}

/* inform kernel of socket data bytes available*/
void notify_data_avail(struct tcpcb_s *cb)
{
    if (cb->bytes_to_push <= 0)
	return;

    notify_sock(cb->sock, TDT_AVAIL_DATA, cb->bytes_to_push);
}

void retval_to_sock(void *sock, int retval)
{
    notify_sock(sock, TDT_RETURN, retval);
}

/* called every ktcp cycle when tcpdevfd data is ready*/
static void tcpdev_bind(void)
{
    struct tdb_bind *db = (struct tdb_bind *)sbuf; /* read from sbuf*/
    struct tcpcb_list_s *n;
    int size;
    __u16 port;
    struct tdb_bind_ret bind_ret;

    if (db->addr.sin_family != AF_INET) {
	retval_to_sock(db->sock,-EINVAL);
	return;
    }

    /* SO_RCVBUF currently only sets listen or connect buffer size, NOT accept size!*/
    size = db->rcv_bufsiz? db->rcv_bufsiz: CB_NORMAL_BUFSIZ;
    n = tcpcb_new(size);
    if (n == NULL) {
	retval_to_sock(db->sock,-ENOMEM);
	return;
    }

    port = ntohs(db->addr.sin_port);
    if (port == 0) {
	if (++next_port < 1024)
	    next_port = 1024;
	while (tcpcb_check_port(next_port) != NULL)
	    next_port++;
	port = next_port;
    } else {
	struct tcpcb_list_s *n2 = tcpcb_check_port(port);
	if (n2) {			/* port already bound */
	    if (!db->reuse_addr) {	/* no SO_REUSEADDR on socket */
		debug_tune("tcp: port %u already bound, rejecting (use SO_REUSEADDR?)\n", port);
reject:
		tcpcb_remove(n);
		retval_to_sock(db->sock, -EADDRINUSE);
		return;
	    }

	    /* remove TCB control block on SO_REUSEADDR to save heap space */
	    if (n2->tcpcb.state == TS_TIME_WAIT) {
		LEAVE_TIME_WAIT(&n2->tcpcb);	/* entered via FIN_WAIT_2 state on FIN rcvd*/
		tcpcb_remove(n2);
		debug_tune("tcp: port %u REUSED, freeing previous socket in time_wait\n", port);
	    } else {
		printf("tcp: port %u NOT reused, previous socket in state %d\n",
			port, n2->tcpcb.state);
		goto reject;
	    }
	}
    }

    n->tcpcb.sock = db->sock;
    n->tcpcb.localaddr = local_ip;
    n->tcpcb.localport = port;
    n->tcpcb.state = TS_CLOSED;

    bind_ret.type = TDT_BIND;
    bind_ret.ret_value = 0;
    bind_ret.sock = db->sock;
    bind_ret.addr_ip = local_ip;
    bind_ret.addr_port = htons(port);
    write(tcpdevfd, &bind_ret, sizeof(bind_ret));
}

static void tcpdev_accept(void)
{
    struct tcpcb_list_s *n,*newn;
    struct tdb_accept *db = (struct tdb_accept *)sbuf; /* read from sbuf*/
    struct tcpcb_s *cb;
    void *  sock = db->sock;
    struct tdb_accept_ret accept_ret;

    n = tcpcb_find_by_sock(sock);
    if (!n || n->tcpcb.state != TS_LISTEN) {
	retval_to_sock(db->sock,-EINVAL);
	return;
    }
    cb = &n->tcpcb;
    newn = tcpcb_find_unaccepted(sock);
    if (!newn) {			/* SYN not yet received by listen*/
	if (db->nonblock)
	    retval_to_sock(db->sock,-EAGAIN);
	else
	    cb->newsock = db->newsock;	/* save new sock in listen CB for later*/
	debug_accept("tcp accept: WAIT (on SYN) sock[%p] saving newsock[%p]\n", sock, db->newsock);
	return;
    }

    /* SYN already received by listen*/
    cb = &newn->tcpcb;			/* get accepted CB*/

    debug_accept("tcpdev accept: ACCEPT (SYN received before accept) sock[%p], using newsock[%p]\n", sock, db->newsock);

    cb->unaccepted = 0;
    cb->sock = db->newsock;
    cb->newsock = 0;			/* clear newsock in accepted CB*/
    n->tcpcb.newsock = 0;		/* clear newsock in listen CB*/

    accept_ret.type = TDT_ACCEPT;
    accept_ret.ret_value = 0;
    accept_ret.sock = sock;		/* report back listen socket*/
    //accept_ret.sock = db->newsock;	/* report back new socket*/
    accept_ret.addr_ip = cb->remaddr;
    accept_ret.addr_port = htons(cb->remport);
    write(tcpdevfd, &accept_ret, sizeof(accept_ret));
}

void tcpdev_notify_accept(struct tcpcb_s *cb)
{
    struct tcpcb_s *listencb;
    struct tcpcb_list_s *lp;
    struct tdb_accept_ret accept_ret;

    debug_tcpdev("tcpdev_notify_accept\n");
    if (!cb->unaccepted)
	return;
    debug_accept("tcpdev notify_accept: sock[%p] searching for newsock[%p]\n",
	cb->sock, cb->newsock);
    if (!(lp = tcpcb_find_by_sock(cb->newsock))) /* search listen CB socket*/
	return;

    /* SYN occured before accept sys call*/
    listencb = &lp->tcpcb;
    debug_accept("tcpdev notify_accept: found listen socket[%p] newsocket[%p]\n",
	listencb->sock, listencb->newsock);

    if (!listencb->newsock)
	return;				/* wait for sys accept as newsock not set yet*/

    accept_ret.type = TDT_ACCEPT;
    accept_ret.ret_value = 0;
    accept_ret.sock = listencb->sock;	/* report back listen socket*/
    //accept_ret.sock = listencb->newsock;	/* report back new socket*/
    accept_ret.addr_ip = cb->remaddr;
    accept_ret.addr_port = htons(cb->remport);

    debug_accept("tcpdev notify_accept: ACCEPT (SYN received before accept) sock[%p] newsock[%p]\n", listencb->sock, listencb->newsock);

    cb->unaccepted = 0;
    cb->sock = listencb->newsock;
    listencb->newsock = 0;

    write(tcpdevfd, &accept_ret, sizeof(accept_ret));
}

static void tcpdev_connect(void)
{
    struct tdb_connect *db = (struct tdb_connect *)sbuf; /* read from sbuf*/
    struct tcpcb_list_s *n;
    ipaddr_t addr;

    n = tcpcb_find_by_sock(db->sock);
    if (!n || n->tcpcb.state != TS_CLOSED) {
	debug_tcp("tcp: panic in connect\n");
	return;
    }

    /* convert localhost to local_ip*/
    addr = db->addr.sin_addr.s_addr;
    if (addr == ntohl(INADDR_LOOPBACK))
	addr = local_ip;
    n->tcpcb.remaddr = addr;
    n->tcpcb.remport = ntohs(db->addr.sin_port);

    if (n->tcpcb.remport == NETCONF_PORT && n->tcpcb.remaddr == 0) {
	n->tcpcb.state = TS_ESTABLISHED;
	notify_sock(n->tcpcb.sock, TDT_CONNECT, 0);	/* success*/
    } else
	tcp_connect(&n->tcpcb);
}

static void tcpdev_listen(void)
{
    struct tdb_listen *db = (struct tdb_listen *)sbuf; /* read from sbuf*/
    struct tcpcb_list_s *n;

    n = tcpcb_find_by_sock(db->sock);
    if (!n || n->tcpcb.state != TS_CLOSED) {
	retval_to_sock(db->sock,-EINVAL);
	return;
    }

    debug_accept("tcp listen: port %u sock[%p]\n", n->tcpcb.localport, db->sock);
    n->tcpcb.state = TS_LISTEN;
    n->tcpcb.newsock = 0;
    retval_to_sock(db->sock, 0);
}

/* kernel read data from ktcp (network)*/
static void tcpdev_read(void)
{
    struct tdb_read *db = (struct tdb_read *)sbuf; /* read/write from sbuf*/
    struct tdb_return_data *ret_data;
    struct tcpcb_list_s *n;
    struct tcpcb_s *cb;
    unsigned int data_avail;
    void * sock = db->sock;

    n = tcpcb_find_by_sock(sock);
    if (!n || n->tcpcb.state == TS_CLOSED) {
	printf("ktcp: panic in read\n");
	return;
    }

    cb = &n->tcpcb;
    if (cb->state == TS_CLOSING || cb->state == TS_LAST_ACK || cb->state == TS_TIME_WAIT) {
	printf("tcpdev_read: returning -EPIPE to socket read state %d\n", cb->state);
	retval_to_sock(sock, -EPIPE);
	return;
    }

    data_avail = cb->bytes_to_push;
    //debug_window("tcpdev_read %u bytes avail %u\n", db->size, data_avail);

    if (data_avail == 0) {
	if (cb->state == TS_CLOSE_WAIT) {
	    printf("tcpdev_read: read on CLOSE_WAIT socket, return -EPIPE\n");
	    retval_to_sock(sock, -EPIPE);
	} else if (db->nonblock)
	    retval_to_sock(sock, -EAGAIN);
	else {
	    //cb->wait_data = db->size;	  /* wait_data use removed, no async reads*/
	    retval_to_sock(sock, -EINTR); /* don't set wait_data, return -EINTR instead*/
	}
	return;
    }

    data_avail = db->size < data_avail ? db->size : data_avail;
    cb->bytes_to_push -= data_avail;
    if (cb->bytes_to_push <= 0)
	tcpcb_need_push--;

    //printf("ktcpdev read: %d bytes\n", data_avail);
    ret_data = (struct tdb_return_data *)sbuf;
    ret_data->type = TDT_RETURN;
    ret_data->ret_value = data_avail;
    ret_data->size = data_avail;
    ret_data->sock = sock;
    tcpcb_buf_read(cb, ret_data->data, data_avail);
    write(tcpdevfd, sbuf, sizeof(struct tdb_return_data) + data_avail);

    /* if remote closed and more data, update data avail then indicate disconnecting*/
    if (cb->state == TS_CLOSE_WAIT) {
	if (cb->bytes_to_push <= 0) {
	    debug_tcp("tcp: disconnecting after final read %d\n", data_avail);
		notify_sock(cb->sock, TDT_CHG_STATE, SS_DISCONNECTING);
	} else {
	    debug_tcp("tcp: application read too small after FIN, data_avail %d\n",
		cb->bytes_to_push);
	    notify_data_avail(cb);	/* inform kernel of remaining data*/
	}
	return;
    }

    /* send ACK to restart server should window have been full (unless it's netstat)*/
    if (cb->remport != NETCONF_PORT || cb->remaddr != 0)
	if (cb->remport != local_ip) {	/* no ack to localhost either*/
	    debug_window("tcp: extra ACK seq %ld, app read %d bytes\n",
		cb->rcv_nxt - cb->irs, data_avail);
	    tcp_send_ack(cb);
	}
}

/* kernel write data to ktcp (network)*/
static void tcpdev_write(void)
{
    struct tdb_write *db = (struct tdb_write *)sbuf; /* read from sbuf*/
    struct tcpcb_list_s *n;
    struct tcpcb_s *cb;
    void *  sock = db->sock;
    unsigned int size, maxwindow;

    sock = db->sock;
    /*
     * Must save db->size as sbuf invalid after call to tcp_output,
     * as when localhost notify_data_avail call uses same sbuf.
     */
    size = db->size;

    /* This is a bit ugly but I'm to lazy right now */
    if (tcp_retrans_memory > TCP_RETRANS_MAXMEM) {
	printf("ktcp: RETRANS memory limit exceeded\n");
	retval_to_sock(sock, -ENOMEM);
	return;
    }

    n = tcpcb_find_by_sock(sock);
    if (!n || n->tcpcb.state == TS_CLOSED) {
	printf("tcpdev_write: write to unknown socket\n");
	retval_to_sock(sock, -EPIPE);
	return;
    }

    cb = &n->tcpcb;

    if (cb->state != TS_ESTABLISHED && cb->state != TS_CLOSE_WAIT) {
	printf("tcpdev_write: write to socket in improper state %d\n", cb->state);
	retval_to_sock(sock, -EPIPE);
	return;
    }

    if (cb->remport == NETCONF_PORT && cb->remaddr == 0) {
	if (db->size == sizeof(struct stat_request_s)) {
	    netconf_request((struct stat_request_s *)db->data);
	    netconf_send(cb);		/* queue response*/
	    notify_data_avail(cb);	/* set sock->data_avail to allow inet_read()*/
	}
	retval_to_sock(sock, size);
	return;
    }

    /* Delay sending if outstanding send window too large. FIXME could hang if no ACKs rcvd*/
    maxwindow = cb->rcv_wnd;
    if (maxwindow > TCP_SEND_WINDOW_MAX)	/* limit retrans memory usage*/
	maxwindow = TCP_SEND_WINDOW_MAX;
    if (cb->send_nxt - cb->send_una + size > maxwindow) {
	debug_tcp("tcp limit: seq %lu size %d maxwnd %u unack %lu rcvwnd %u\n",
	    cb->send_nxt - cb->iss, size, maxwindow, cb->send_nxt - cb->send_una, cb->rcv_wnd);
	retval_to_sock(sock, -ERESTARTSYS);	/* kernel will retry 100ms later*/
	return;
    }

    debug_tcp("tcp write: seq %lu size %d rcvwnd %u unack %lu (cnt %d, mem %u)\n",
	cb->send_nxt - cb->iss, size, cb->rcv_wnd, cb->send_nxt - cb->send_una,
	tcp_timeruse, tcp_retrans_memory);

    cb->flags = TF_PSH|TF_ACK;
    cb->datalen = size;
    cb->data = db->data;
    tcp_output(cb);

    retval_to_sock(sock, size);
}

static void tcpdev_release(void)
{
    struct tdb_release *db = (struct tdb_release *)sbuf; /* read from sbuf*/
    struct tcpcb_list_s *n;
    struct tcpcb_s *cb;
    void * sock = db->sock;

    n = tcpcb_find_by_sock(sock);
    if (n) {
	cb = &n->tcpcb;
	debug_close("tcpdev release: close socket %p, state is %s\n",
	    sock, tcp_states[cb->state]);

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
		debug_close("tcp[%p] setting state to FIN_WAIT_1\n", cb->sock);
		cb->state = TS_FIN_WAIT_1;
		goto common_close;
	    case TS_CLOSE_WAIT:
		debug_close("tcp[%p] setting state to LAST_ACK\n", cb->sock);
		cb->state = TS_LAST_ACK;
common_close:
		if (db->reset) {		/* SO_LINGER w/zero timer */
		   tcp_reset_connection(cb);	/* send RST and deallocate */
		} else {
		    cbs_in_user_timeout++;
		    cb->time_wait_exp = Now;
		    tcp_send_fin(cb);
		}
		break;
	    default:
		debug_close("tcp[%p] NO state change from %s on release\n",
		    cb->sock, tcp_states[cb->state]);
		break;
	}
    }
}

void tcpdev_process(void)
{
	int len = read(tcpdevfd, sbuf, TCPDEV_BUFSIZ);
	if (len <= 0)
		return;

	debug_tcpdev("tcpdev_process read %d bytes\n",len);

	switch (sbuf[0]){
	case TDC_BIND:
	    debug_tcpdev("tcpdev_bind\n");
	    tcpdev_bind();
	    break;
	case TDC_ACCEPT:
	    debug_tcpdev("tcpdev_accept\n");
	    tcpdev_accept();
	    break;
	case TDC_CONNECT:
	    debug_tcpdev("tcpdev_connect\n");
	    tcpdev_connect();
	    break;
	case TDC_LISTEN:
	    debug_tcpdev("tcpdev_listen\n");
	    tcpdev_listen();
	    break;
	case TDC_RELEASE:
	    debug_tcpdev("tcpdev_release\n");
	    tcpdev_release();
	    break;
	case TDC_READ:
	    debug_tcpdev("tcpdev_read\n");
	    tcpdev_read();
	    break;
	case TDC_WRITE:
	    debug_tcpdev("tcpdev_write\n");
	    tcpdev_write();
	    break;
	}
}
