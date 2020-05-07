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

#include <linuxmt/arpa/inet.h>

#include "deveth.h"
#include "ip.h"
#include "arp.h"


/* ARP operations */
/* as big endian values */

#define ARP_REQUEST  0x0100
#define ARP_REPLY    0x0200


/* Local ARP cache */

#define ARP_CACHE_MAX 5

struct arp_cache_s {
	ipaddr_t   ip_addr;   /* IPv4 address */
	eth_addr_t eth_addr;  /* MAC address */
	};

typedef struct arp_cache_s arp_cache_t;

static arp_cache_t _arp_cache [ARP_CACHE_MAX];


static void arp_cache_init (void)
{
	memset (_arp_cache, 0, ARP_CACHE_MAX * sizeof (arp_cache_t));
}


int arp_cache_get (ipaddr_t ip_addr, eth_addr_t * eth_addr)
{
	int err = -1;

	/* First pair is the more recent */

	arp_cache_t * entry = _arp_cache;
	while (entry < _arp_cache + ARP_CACHE_MAX) {
		if (!entry->ip_addr) break;

		if (entry->ip_addr == ip_addr) {
			memcpy (eth_addr, entry->eth_addr, sizeof (eth_addr_t));
			err = 0;
			break;
		}

		entry++;
	}

	return err;
}


void arp_cache_add (ipaddr_t ip_addr, eth_addr_t * eth_addr)
{
	if (arp_cache_get (ip_addr, eth_addr)) {
		/* Shift the whole cache */

		arp_cache_t * entry = _arp_cache + ARP_CACHE_MAX - 1;
		while (entry > _arp_cache) {
			memcpy (entry, entry - 1, sizeof (arp_cache_t));
			entry--;
		}

		/* Set the first pair as the more recent */

		entry->ip_addr = ip_addr;
		memcpy (entry->eth_addr, eth_addr, sizeof (eth_addr_t));
	}
}


void arp_print(struct arp *arp_r)
{
    __u8 *addr;

    addr = arp_r->ll_eth_src;
    printf("ll_eth_src: %2X.%2X.%2X.%2X.%2X.%2X ",addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
    addr = arp_r->ll_eth_dest;
    printf("ll_eth_dest: %2X.%2X.%2X.%2X.%2X.%2X \n",addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);

    printf("Op: %2X \n",arp_r->op);

    addr = (__u8 *)&arp_r->ip_src;
    printf("ip_src : %d.%d.%d.%d ",addr[0],addr[1],addr[2],addr[3]);
    addr = (__u8 *)&arp_r->ip_dest;
    printf("ip_dest: %d.%d.%d.%d \n",addr[0],addr[1],addr[2],addr[3]);
    addr = arp_r->eth_src;
    printf("eth_src: %2X.%2X.%2X.%2X.%2X.%2X ",addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
    addr = arp_r->eth_dest;
    printf("eth_dest: %2X.%2X.%2X.%2X.%2X.%2X \n",addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
}

int arp_init (void)
{
	arp_cache_init ();
	return 0;
}

void arp_reply(char *packet,int size)
{
    struct arp_addr apair;
    struct arp *arp_r;

    arp_r = (struct arp *) packet;

    /* arp_print(arp_r); */

    /* swap ip addresses and mac addresses */
    apair.daddr = arp_r->ip_src;
    apair.saddr = arp_r->ip_dest;
    memcpy(apair.eth_dest, arp_r->eth_src, 6);
    memcpy(apair.eth_src, eth_local_addr, 6);

    /* build arp reply */
    arp_r->op=0x200; /*response - big endian*/
    memcpy(arp_r->ll_eth_dest, apair.eth_dest, 6);
    memcpy(arp_r->ll_eth_src, apair.eth_src, 6);

    memcpy(arp_r->eth_src, apair.eth_src, 6);
    arp_r->ip_src=apair.saddr;
    memcpy(arp_r->eth_dest, apair.eth_dest, 6);
    arp_r->ip_dest=apair.daddr;

    deveth_send(packet, sizeof (struct arp));
}

int arp_request(ipaddr_t ipaddress)
{
    int i;
    fd_set fdset;
    struct arp *arp_r;
    static char packet[sizeof(struct arp)];
    arp_r = (struct arp *)packet;

    /* build arp request */
    for (i=0;i<6;i++) arp_r->ll_eth_dest[i]=0xFF; /*broadcast*/
    memcpy(arp_r->ll_eth_src, eth_local_addr, 6);
    /*specify below in big endian*/
    arp_r->ll_type_len=0x0608;
    arp_r->hard_type=0x0100;
    arp_r->proto_type=0x0008;
    arp_r->hard_len=6;
    arp_r->proto_len=4;
    arp_r->op=0x0100; /*request - big endian*/
    memcpy(arp_r->eth_src, eth_local_addr, 6);
    arp_r->ip_src=local_ip;
    for (i=0;i<6;i++) arp_r->eth_dest[i]=0;
    arp_r->ip_dest=ipaddress;

    deveth_send(&packet, sizeof (struct arp));

    /* wait for reply */
selectagain:
    FD_ZERO(&fdset);
    FD_SET(tcpdevfd, &fdset);
    i = select(tcpdevfd + 1, &fdset, NULL, NULL, NULL);
    if (i < 0) {
	if (errno == EINTR) {
		fprintf(stderr, "arp: select EINTR\n");
		goto selectagain;
	}
	perror("select");
	return -1;
    }
    deveth_process(); /*get reply*/

    return 0;
}


/* Process incoming ARP packets */

void arp_proc (char * packet, int size)
{
	struct arp * arp_r;

	arp_r = (struct arp *) packet;
	switch (arp_r->op) {
	case ARP_REQUEST:
		arp_cache_add (arp_r->ip_src, arp_r->eth_src);
		arp_reply (packet, size);
		break;

	case ARP_REPLY:
		arp_cache_add (arp_r->ip_src, arp_r->eth_src);
		break;
	}
}
