/*
 * This file is part of the ELKS TCP/IP stack
 *
 * 13 Jul 20 Greg Haerr - added eth_route to handle ARP requests and packet queuing
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <linuxmt/limits.h>
#include <arch/ioctl.h>

#include "config.h"
#include "tcp.h"
#include "ip.h"
#include "deveth.h"
#include "tcpdev.h"
#include "arp.h"
#include "netconf.h"

eth_addr_t eth_local_addr;

static unsigned char sbuf[MAX_PACKET_ETH];
static int devfd;
//static eth_addr_t broad_addr = {255, 255, 255, 255, 255, 255};

int deveth_init(char *fdev)
{
    devfd = open(fdev, O_NONBLOCK|O_RDWR);
    if (devfd < 0) {
	printf("ktcp: failed to open eth device %s\n", fdev);
	return -1;
    }

    /* read mac of nic */
    if (ioctl (devfd, IOCTL_ETH_ADDR_GET, eth_local_addr) < 0) {
        printf("ktcp: IOCTL_ETH_ADDR_GET fail\n");

        /* MAC not available is a fatal error */
        /* because it means the driver does not work */

        return -2;
    }

    return devfd;
}


/*
 *  Called when select in ktcp indicates we have new data waiting
 */
void eth_process(void)
{
  eth_head_t * eth_head;
  int len = read (devfd, sbuf, MAX_PACKET_ETH);
  if (len < (int)sizeof(eth_head_t)) {
	if (len < 0) printf("ktcp: eth_process error %d, discarding packet\n", len); //FIXME
	return;
  }
  if (len > (int)sizeof(sbuf)) { printf("ktcp: eth_process OVERFLOW\n"); exit(1); }

  eth_head = (eth_head_t *) sbuf;

#if 0
  /* Filter on MAC addresses in case of promiscuous mode*/
  if (memcmp (eth_head->eth_dst, broad_addr, sizeof (eth_addr_t))
    && memcmp (eth_head->eth_dst, eth_local_addr, sizeof (eth_addr_t)))
      return;
#endif

  /* dispatch to IP or ARP*/
  switch (eth_head->eth_type) {
  case ETH_TYPE_IPV4:
	  /* strip link layer */
	  ip_recvpacket (sbuf + sizeof(eth_head_t), len - sizeof(eth_head_t));
	  break;

  case ETH_TYPE_ARP:
	  arp_recvpacket (sbuf, len);
	  break;
  }
  netstats.ethrcvcnt++;
}

/*
 * Determine ethernet address for IP packet using ARP request/cache
 * Packet will be sent if address cached, otherwise sent after ARP reply
 * Only one packet will be queued per remote host waiting for ARP reply, others dropped
 */
void eth_route(unsigned char *packet, int len, ipaddr_t ip_addr)
{
	struct arp_cache *entry;
	eth_addr_t eth_addr;
	unsigned char *p;

	/* try to get cached ethernet address and send packet*/
	if (arp_cache_get (ip_addr, eth_addr, ARP_VALID)) {
		eth_sendpacket(packet, len, eth_addr);
		return;
	}

	/* no address in cache, create holding entry, queue packet and send ARP request*/
	entry = arp_cache_add(ip_addr, NULL);
	if (entry->qpacket) {
		/* TCP packet will auto retrans, 2+ ICMP will be lost until ARP reply seen*/
		printf("eth: DROPPING packet to %s, one already queued awaiting ARP reply\n",
			in_ntoa(ip_addr));
		arp_request(ip_addr);		/* resend ARP request*/
		return;
	}

	/* allocate memory and queue packet*/
	if ((p = malloc(len + sizeof(struct ip_ll)))) {
		memcpy(p+sizeof(struct ip_ll), packet, len);
		entry->qpacket = p + sizeof(struct ip_ll);
		entry->len = len;
		debug_arp("eth: queueing packet len %d\n", len);
	}
	arp_request(ip_addr);
}

/*
 * Send IP packet after prepending ethernet header
 */
void eth_sendpacket(unsigned char *packet, int len, eth_addr_t eth_addr)
{
	/* space guaranteed before packet for ethernet header by ip_sendpacket()/ip_route()*/
	struct ip_ll *ipll = (struct ip_ll *)(packet - sizeof(struct ip_ll));

	/* add link layer*/
	memcpy(ipll->ll_eth_dest, eth_addr, 6);
	memcpy(ipll->ll_eth_src, eth_local_addr, 6);
	ipll->ll_type_len = 0x08; //FIXME what is 0x0800

	eth_write((unsigned char *)ipll, sizeof(struct ip_ll) + len);
}

/* raw ethernet packet send*/
void eth_write(unsigned char *packet, int len)
{
#if DEBUG_ETH
    eth_printhex(packet,len);
#endif
    write(devfd, packet, len);
    netstats.ethsndcnt++;
}

#if DEBUG_ETH
void eth_printhex(unsigned char *packet, int len)
{
  unsigned char *p = packet;
  int i = 0;
  printf("eth: write %d bytes: ", len);
  if (len > 128) len = 128;
  while (len--) {
	printf("%02X ", *p++);
	if ((i++ & 15) == 15) printf("\n");
  }
  printf("\n");
}
#endif
