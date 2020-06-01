/*
 * This file is part of the ELKS TCP/IP stack
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
#include <errno.h>
#include <time.h>

#include <linuxmt/arpa/inet.h>

#include "deveth.h"
#include "tcp.h"
#include "tcpdev.h"
#include "ip.h"
#include "arp.h"


/* ARP operations */
#define ARP_REQUEST  1
#define ARP_REPLY    2

/* Local ARP cache */
#define ARP_CACHE_MAX 5

struct arp_cache_s {
	ipaddr_t   ip_addr;   /* IPv4 address */
	eth_addr_t eth_addr;  /* MAC address */
};

typedef struct arp_cache_s arp_cache_t;

static arp_cache_t arp_cache [ARP_CACHE_MAX];


static void arp_cache_init (void)
{
	memset (arp_cache, 0, ARP_CACHE_MAX * sizeof (arp_cache_t));
}


int arp_cache_get (ipaddr_t ip_addr, eth_addr_t * eth_addr)
{
	register arp_cache_t * entry = arp_cache;

	/* First pair is the more recent */
	while (entry < arp_cache + ARP_CACHE_MAX) {
		if (!entry->ip_addr)
		    break;
		if (entry->ip_addr == ip_addr) {
			memcpy (eth_addr, entry->eth_addr, sizeof (eth_addr_t));
			debug_arp("arp: using cached entry for %s\n", in_ntoa(ip_addr));
			return 0;
		}
		entry++;
	}

	debug_arp("arp: no cached entry for %s\n", in_ntoa(ip_addr));
	return 1;
}


void arp_cache_add (ipaddr_t ip_addr, eth_addr_t * eth_addr)
{
	if (arp_cache_get (ip_addr, eth_addr)) {

		/* Shift the whole cache */
		arp_cache_t * entry = arp_cache + ARP_CACHE_MAX - 1;
		while (entry > arp_cache) {
			memcpy (entry, entry - 1, sizeof (arp_cache_t));
			entry--;
		}

		/* Set the first pair as the more recent */
		entry->ip_addr = ip_addr;
		memcpy (entry->eth_addr, eth_addr, sizeof (eth_addr_t));
		debug_arp("arp: adding cache entry for %s\n", in_ntoa(ip_addr));
	} else
		debug_arp("arp: already cache entry for %s, ignored\n", in_ntoa(ip_addr)); //FIXME
}


char *mac_ntoa(unsigned char *p)
{
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

int arp_init (void)
{
	arp_cache_init ();
	return 0;
}

void arp_reply(unsigned char *packet,int size)
{
    register struct arp *arp = (struct arp *)packet;
    struct arp_addr apair;

    debug_arp("arp: SEND reply to %s\n", in_ntoa(arp->ip_src));

    /* swap ip addresses and mac addresses */
    apair.daddr = arp->ip_src;
    apair.saddr = arp->ip_dest;
    memcpy(apair.eth_dest, arp->eth_src, 6);
    memcpy(apair.eth_src, eth_local_addr, 6);

    /* build arp reply */
    arp->op = htons(ARP_REPLY);
    memcpy(arp->ll_eth_dest, apair.eth_dest, 6);
    memcpy(arp->ll_eth_src, apair.eth_src, 6);

    memcpy(arp->eth_src, apair.eth_src, 6);
    arp->ip_src=apair.saddr;
    memcpy(arp->eth_dest, apair.eth_dest, 6);
    arp->ip_dest=apair.daddr;

    arp_print(arp);
    deveth_send(packet, sizeof (struct arp));
}

int arp_request(ipaddr_t ipaddress)
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
    deveth_send((unsigned char *)&arpreq, sizeof(arpreq));

    /* timeout wait for reply */
    debug_arp("arp: wait reply\n");
    struct timeval timeint;
    fd_set fdset;
    timeint.tv_sec  = 0;
    timeint.tv_usec = 200000L;	/* 200 msec*/
    FD_ZERO(&fdset);
    FD_SET(tcpdevfd, &fdset);
    int i = select(tcpdevfd + 1, &fdset, NULL, NULL, &timeint);
    if (i >= 0) {
	printf("arp: got reply on timed wait\n");
	deveth_process(1);
	return 0;	/* success*/
    }
    return 1;		/* error*/
}

/* Process incoming ARP packets */
void arp_recvpacket(unsigned char *packet, int size)
{
	register struct arp *arp = (struct arp *)packet;

	arp_print(arp);
	switch (ntohs(arp->op)) {
	case ARP_REQUEST:
		debug_arp("arp: incoming REQUEST\n");
		arp_cache_add (arp->ip_src, &arp->eth_src);
		arp_reply (packet, size);
		break;

	case ARP_REPLY:
		debug_arp("arp: incoming REPLY\n");
		arp_cache_add (arp->ip_src, &arp->eth_src);
		break;
	}
}
