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
#include "netorder.h"
#include "tcp.h"
#include "tcpdev.h"
#include "netconf.h"


void netconf_init(void)
{
    /* Do nothing */
}

static struct stat_request_s sreq;

void netconf_request(struct stat_request_s *sr)
{
    sreq.type = sr->type;
    sreq.extra = sr->extra;		
}

void netconf_send(struct tcpcb_s *cb)
{
#ifdef CONFIG_INET_STATUS
    struct general_stats_s gstats;
    struct cb_stats_s cbstats;
    struct tcpcb_s *ncb;

    if (sreq.type == NS_GENERAL) {
	gstats.cb_num = tcpcb_num;
	gstats.retrans_memory = tcp_retrans_memory;
	tcpcb_buf_write(cb, &gstats, sizeof(gstats));
    } else if (sreq.type == NS_CB) {
	ncb = tcpcb_getbynum(sreq.extra);
	if (ncb) {
	    cbstats.valid = 1;
	    cbstats.state = ncb->state;
	    cbstats.rtt = ncb->rtt * 1000 / 16;
	    cbstats.remaddr = ncb->remaddr;
	    cbstats.remport = ncb->remport;
	    cbstats.localport = ncb->localport;
	} else
	    cbstats.valid = 0;
	tcpcb_buf_write(cb, &cbstats, sizeof(cbstats) );
    }
#endif
    cb->bytes_to_push = CB_BUF_USED(cb);
}
