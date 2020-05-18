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
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <linuxmt/limits.h>
#include <arch/ioctl.h>

#include "config.h"
#include "deveth.h"
#include "tcp.h"
#include "ip.h"
#include "tcpdev.h"
#include "arp.h"

eth_addr_t eth_local_addr;
int eth_device;

static unsigned char sbuf [MAX_PACKET_ETH];
static int devfd;

static eth_addr_t broad_addr = {255, 255, 255, 255, 255, 255};


#if DEBUG
void deveth_printhex(unsigned char *packet, int len)
{
  unsigned char *p = packet;
  int i = 0;
  printf("deveth_process():%d bytes\n",len);
  if (len > 128) len = 128;
  while (len--) {
	printf("%02X ", *p++);
	if ((i++ & 15) == 15) printf("\n");
  }
  printf("\n");
}
#endif

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

    debug_arp("eth_local_addr: %s\n", mac_ntoa(&eth_local_addr));

    return devfd;
}


/*
 *  Called when select in ktcp indicates we have new data waiting
 */

void deveth_process(void)
{
  eth_head_t * eth_head;
  int len = read (devfd, sbuf, MAX_PACKET_ETH);
  if (len < sizeof(eth_head_t))
	return;

  eth_head = (eth_head_t *) sbuf;

  /* Filter on MAC addresses */
  /* in case of promiscuous mode */

  if (memcmp (eth_head->eth_dst, broad_addr, sizeof (eth_addr_t))
    && memcmp (eth_head->eth_dst, eth_local_addr, sizeof (eth_addr_t)))
      return;

  /* Decode Ethernet II header */
  /* and dispatch to protocols */

  switch (eth_head->eth_type) {
  case ETH_TYPE_IPV4:
	  /* strip link layer */
	  ip_recvpacket (sbuf + sizeof(eth_head_t), len - sizeof(eth_head_t));
	  break;

  case ETH_TYPE_ARP:
	  arp_recvpacket (sbuf, len);
	  break;
  }
}


void deveth_send(unsigned char *packet, int len)
{
    //deveth_printhex(packet,len);
    write(devfd, packet, len);
}
