/*
 * This file is part of the ELKS TCP/IP stack
 *
 * 13 Jul 20 Greg Haerr - rewrote ARP handling, ARP and IP packets handled in main loop
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <arpa/inet.h>
#include "tcp.h"
#include "tcpdev.h"
#include "ip.h"
#include "deveth.h"
#include "arp.h"
#include "netconf.h"

struct arp_cache arp_cache [ARP_CACHE_MAX];

int arp_init (void)
{
	memset (arp_cache, 0, ARP_CACHE_MAX * sizeof (struct arp_cache));
	return 0;
}

struct arp_cache *arp_cache_get(ipaddr_t ip_addr, eth_addr_t eth_addr, int flags)
{
	register struct arp_cache *entry = arp_cache;

	/* First pair is the more recent */
	while (entry < arp_cache + ARP_CACHE_MAX) {
		if (!entry->ip_addr)
		    break;
		if (entry->ip_addr == ip_addr) {
			if ((flags & ARP_VALID) && !entry->valid)
				return NULL;	/* not yet valid - awaiting ARP reply*/
			if (flags & ARP_UPDATE) {
				memcpy (entry->eth_addr, eth_addr, sizeof (eth_addr_t));
				debug_arp("arp: merging cached entry for %s (%s)\n",
					in_ntoa(ip_addr), mac_ntoa(entry->eth_addr));
			} else {
				memcpy (eth_addr, entry->eth_addr, sizeof (eth_addr_t));
				debug_arp("arp: using cached entry for %s (%s)\n",
					in_ntoa(ip_addr),mac_ntoa(entry->eth_addr));
			}
			return entry;	/* success*/
		}
		entry++;
	}

	debug_arp("arp: no cached entry for %s\n", in_ntoa(ip_addr));
	return NULL;			/* not found*/
}

struct arp_cache *arp_cache_update(ipaddr_t ip_addr, eth_addr_t eth_addr)
{
	struct arp_cache *entry;
	eth_addr_t existing_addr;

	if ((entry = arp_cache_get(ip_addr, existing_addr, 0))) {
		memcpy (entry->eth_addr, eth_addr, sizeof (eth_addr_t));
		debug_arp("arp: updating cached entry for %s (%s)\n",
			in_ntoa(entry->ip_addr), mac_ntoa(entry->eth_addr));
		entry->valid = 1;
	} else debug_arp("arp: no cached entry to update for %s\n", in_ntoa(ip_addr));

	return entry;
}

struct arp_cache *arp_cache_add(ipaddr_t ip_addr, eth_addr_t eth_addr)
{
	struct arp_cache *entry;

	/* Shift the whole cache */
	entry = arp_cache + ARP_CACHE_MAX - 1;
	while (entry > arp_cache) {
		memcpy (entry, entry - 1, sizeof (struct arp_cache));
		entry--;
	}

	/* Set the first entry as the more recent */
	entry->ip_addr = ip_addr;
	if (eth_addr) {
		memcpy (entry->eth_addr, eth_addr, sizeof (eth_addr_t));
		entry->valid = 1;
	} else entry->valid = 0;	/* no MAC address yet, awaiting ARP reply*/
	entry->qpacket = NULL;
	debug_arp("arp: adding cache entry for %s, valid=%d\n", in_ntoa(ip_addr), entry->valid);

	netstats.arpcacheadds++;
	return entry;
}

char *mac_ntoa(eth_addr_t eth_addr)
{
	unsigned char *p = (unsigned char *)eth_addr;
    static char b[18];

    sprintf(b, "%02x.%02x.%02x.%02x.%02x.%02x",p[0],p[1],p[2],p[3],p[4],p[5]);
    return b;
}

static void arp_print(register struct arp *arp)
{
#if DEBUG_ARP
    printf("ARP: ");
    printf("op:%d ", ntohs(arp->op));
    printf("ll:%s->", mac_ntoa(arp->ll_eth_src));
    printf("%s ", mac_ntoa(arp->ll_eth_dest));
    printf("%s->", in_ntoa(arp->ip_src));
    printf("%s ", in_ntoa(arp->ip_dest));
    printf("eth:%s->", mac_ntoa(arp->eth_src));
    printf("%s", mac_ntoa(arp->eth_dest));
    printf("\n");
#endif
}

void arp_reply(unsigned char *packet,int size)
{
    register struct arp *arp = (struct arp *)packet;
    struct arp_addr swap;

    debug_arp("arp: SEND reply to %s\n", in_ntoa(arp->ip_src));

    /* swap ip addresses and mac addresses */
    swap.daddr = arp->ip_src;
    swap.saddr = arp->ip_dest;
    memcpy(swap.eth_dest, arp->eth_src, 6);
    memcpy(swap.eth_src, eth_local_addr, 6);

    /* build arp reply */
    arp->op = htons(ARP_REPLY);
    memcpy(arp->ll_eth_dest, swap.eth_dest, 6);
    memcpy(arp->ll_eth_src, swap.eth_src, 6);

    memcpy(arp->eth_src, swap.eth_src, 6);
    arp->ip_src = swap.saddr;
    memcpy(arp->eth_dest, swap.eth_dest, 6);
    arp->ip_dest = swap.daddr;

    arp_print(arp);
    eth_write(packet, sizeof (struct arp));
    netstats.arpsndreplycnt++;
}

void arp_request(ipaddr_t ipaddress)
{
    struct arp arpreq;

    debug_arp("arp: SEND request\n");

    /* build arp request */
    memset(arpreq.ll_eth_dest, 0xFF, 6);	/* broadcast*/
    memcpy(arpreq.ll_eth_src, eth_local_addr, 6);
    /*specify below in big endian*/ //FIXME
    arpreq.ll_type_len = ETH_TYPE_ARP;
    arpreq.hard_type=0x0100;
    arpreq.proto_type = ETH_TYPE_IPV4;
    arpreq.hard_len=6;
    arpreq.proto_len=4;
    arpreq.op = htons(ARP_REQUEST);
    memcpy(arpreq.eth_src, eth_local_addr, 6);
    arpreq.ip_src=local_ip;
    memset(arpreq.eth_dest, 0, 6);
    arpreq.ip_dest=ipaddress;

    arp_print(&arpreq);
    eth_write((unsigned char *)&arpreq, sizeof(arpreq));
    netstats.arpsndreqcnt++;
}

/* Process incoming ARP packets */
void arp_recvpacket(unsigned char *packet, int size)
{
	register struct arp *arp = (struct arp *)packet;
	struct arp_cache *entry;

	arp_print(arp);
	switch (ntohs(arp->op)) {
	case ARP_REQUEST:
		debug_arp("arp: incoming REQUEST\n");
		entry = arp_cache_get(arp->ip_src, arp->eth_src, ARP_UPDATE); /* possible cache update */
		if (arp->ip_dest == local_ip) {
			if (!entry)
				arp_cache_add(arp->ip_src, arp->eth_src);
			arp_reply (packet, size);
		}
		netstats.arprcvreqcnt++;
		break;

	case ARP_REPLY:
		debug_arp("arp: incoming REPLY\n");
		/* update cache, check for queued packet*/
		entry = arp_cache_update(arp->ip_src, arp->eth_src);
		if (entry) {
			if (entry->qpacket) {
				debug_arp("arp: sending queued packet\n");
				eth_sendpacket(entry->qpacket, entry->len, entry->eth_addr);
				free(entry->qpacket - sizeof(struct ip_ll));
				entry->qpacket = NULL;
			}
		}
		netstats.arprcvreplycnt++;
		break;
	}
}
