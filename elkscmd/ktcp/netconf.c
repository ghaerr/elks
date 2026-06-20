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

#include <arpa/inet.h>
#include <string.h>
#include "config.h"
#include "tcp.h"
#include "tcp_cb.h"
#include "tcpdev.h"
#include "deveth.h"
#include "arp.h"
#include "icmp.h"
#include "ip.h"
#include "netconf.h"

struct packet_stats_s netstats;
static struct stat_request_s sreq;
static struct icmp_echo_request_s icmp_req;
static struct icmp_traceroute_request_s icmp_tr_req;
static ipaddr_t set_ip_value;		/* temp storage for NS_SET_* operations */
struct tcpcb_s *pending_icmp_cb;	/* netconf client awaiting ICMP echo reply */
int pending_is_traceroute;		/* pending_icmp_cb expects traceroute reply */

void netconf_init(void)
{
    /* Do nothing */
}

void netconf_request(struct stat_request_s *sr)
{
    sreq.type = sr->type;
    sreq.extra = sr->extra;
}

/* Save extra request data following the 2-byte stat_request_s header.
 * Was defined but never called, ICMP echo (and IP/netmask/gateway set)
 * silently dropped their payload.
 */
void netconf_set_extra(unsigned char *data, int len)
{
    if (sreq.type == NS_ICMP_ECHO &&
	len >= (int)(sizeof(struct stat_request_s) +
		sizeof(struct icmp_echo_request_s))) {
	memcpy(&icmp_req, data + sizeof(struct stat_request_s),
		sizeof(struct icmp_echo_request_s));
    } else if (sreq.type == NS_ICMP_TRACEROUTE &&
	len >= (int)(sizeof(struct stat_request_s) +
		sizeof(struct icmp_traceroute_request_s))) {
	memcpy(&icmp_tr_req, data + sizeof(struct stat_request_s),
		sizeof(struct icmp_traceroute_request_s));
    } else if (len >= (int)(sizeof(struct stat_request_s) + sizeof(ipaddr_t))) {
	if (sreq.type == NS_SET_IP || sreq.type == NS_SET_NETMASK ||
		sreq.type == NS_SET_GATEWAY)
	    memcpy(&set_ip_value, data + sizeof(struct stat_request_s), sizeof(ipaddr_t));
    }
}

/* send netstat status*/
void netconf_send(struct tcpcb_s *cb)
{
    struct general_stats_s gstats;
    struct cb_stats_s cbstats;
    struct tcpcb_s *ncb;
    struct config_info_s config;

    switch (sreq.type) {
    case NS_GENERAL:
	gstats.cb_num = tcpcb_num;
	gstats.retrans_memory = tcp_retrans_memory;
	tcpcb_buf_write(cb, (unsigned char *)&gstats, sizeof(gstats));
	break;
    case NS_CB:
	ncb = tcpcb_getbynum(sreq.extra);
	if (ncb) {
	    cbstats.valid = 1;
	    cbstats.state = ncb->state;
	    cbstats.rtt = ncb->rtt * 1000 / 16;
	    cbstats.remaddr = ncb->remaddr;
	    cbstats.remport = ncb->remport;
	    cbstats.localport = ncb->localport;
		cbstats.time_wait_exp = ncb->time_wait_exp;
	} else
	    cbstats.valid = 0;
	tcpcb_buf_write(cb, (unsigned char *)&cbstats, sizeof(cbstats));
	break;
    case NS_NETSTATS:
	tcpcb_buf_write(cb, (unsigned char *)&netstats, sizeof(netstats));
	break;
    case NS_ARP:
	tcpcb_buf_write(cb, (unsigned char *)&arp_cache, ARP_CACHE_MAX*sizeof(struct arp_cache));
	break;
    case NS_ICMP_ECHO:
	/* Send ICMP Echo Request, defer reply until it arrives via
	 * icmp_process() → netconf_icmp_reply(). We cannot block here
	 * because we're in the ktcp event loop, so the reply path is async.
	 * Set pending_icmp_cb BEFORE icmp_send_echo() so localhost loopback
	 * (synchronous via ip_route) finds the TCB when icmp_process handles
	 * the echo reply.
	 */
	pending_is_traceroute = 0;
	pending_icmp_cb = cb;
	icmp_send_echo(icmp_req.target_ip, icmp_req.id, icmp_req.seq,
		icmp_req.timestamp, 64);
	return;
    case NS_ICMP_TRACEROUTE:
	pending_is_traceroute = 1;
	pending_icmp_cb = cb;
	icmp_send_echo(icmp_tr_req.target_ip, icmp_tr_req.id, icmp_tr_req.seq,
		icmp_tr_req.timestamp, icmp_tr_req.ttl);
	return;
    case NS_GET_CONFIG:
	config.local_ip = local_ip;
	config.netmask_ip = netmask_ip;
	config.gateway_ip = gateway_ip;
	memcpy(config.hwaddr, eth_local_addr, 6);
	strncpy(config.name, ethdev + 5, sizeof(config.name) - 1);
	config.name[sizeof(config.name) - 1] = '\0';
	tcpcb_buf_write(cb, (unsigned char *)&config, sizeof(config));
	break;
    case NS_SET_IP:
	local_ip = set_ip_value;
	tcpcb_buf_write(cb, (unsigned char *)"", 1);
	break;
    case NS_SET_NETMASK:
	netmask_ip = set_ip_value;
	tcpcb_buf_write(cb, (unsigned char *)"", 1);
	break;
    case NS_SET_GATEWAY:
	gateway_ip = set_ip_value;
	tcpcb_buf_write(cb, (unsigned char *)"", 1);
	break;
    }
    cb->bytes_to_push = cb->buf_used;
    tcpcb_need_push++;
}

/* Called from icmp_process() when Echo Reply arrives for pending netconf client.
 * The reply path is async: icmp_send_echo() fires the request, and when the ICMP
 * layer receives the reply it calls here to push data back through the netconf socket. */
void netconf_icmp_reply(struct tcpcb_s *cb, unsigned long timestamp, unsigned int ttl)
{
    struct icmp_echo_reply_s reply;

    reply.status = ICMP_ECHO_REPLY_SUCCESS;
    reply.timestamp = timestamp;
    reply.ttl = ttl;

    tcpcb_buf_write(cb, (unsigned char *)&reply, sizeof(reply));
    cb->bytes_to_push = cb->buf_used;
    tcpcb_need_push++;
    notify_data_avail(cb);

    pending_icmp_cb = NULL;
    pending_is_traceroute = 0;
}

/* Called from icmp_process() when traceroute response arrives
 * (Echo Reply or Time Exceeded) for pending netconf client. */
void netconf_icmp_traceroute_reply(struct tcpcb_s *cb, unsigned long timestamp,
	unsigned int ttl, ipaddr_t resp_ip, unsigned int status)
{
    struct icmp_traceroute_reply_s reply;

    reply.status = status;
    reply.timestamp = timestamp;
    reply.ttl = ttl;
    reply.resp_ip = resp_ip;

    tcpcb_buf_write(cb, (unsigned char *)&reply, sizeof(reply));
    cb->bytes_to_push = cb->buf_used;
    tcpcb_need_push++;
    notify_data_avail(cb);

    pending_icmp_cb = NULL;
    pending_is_traceroute = 0;
}
