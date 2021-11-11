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
#include "config.h"
#include "tcp.h"
#include "tcp_cb.h"
#include "tcpdev.h"
#include "deveth.h"
#include "arp.h"
#include "netconf.h"

struct packet_stats_s netstats;
static struct stat_request_s sreq;

void netconf_init(void)
{
    /* Do nothing */
}

void netconf_request(struct stat_request_s *sr)
{
    sreq.type = sr->type;
    sreq.extra = sr->extra;
}

/* send netstat status*/
void netconf_send(struct tcpcb_s *cb)
{
    struct general_stats_s gstats;
    struct cb_stats_s cbstats;
    struct tcpcb_s *ncb;

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
    }
    cb->bytes_to_push = cb->buf_used;
}
