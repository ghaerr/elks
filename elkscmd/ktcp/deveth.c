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
#include <fcntl.h>

#include <linuxmt/limits.h>
#include <arch/ioctl.h>

#include "config.h"
#include "deveth.h"
#include "tcpdev.h"
#include "arp.h"

eth_addr_t eth_local_addr;

static unsigned char sbuf [MAX_PACKET_ETH];
static int devfd;

static eth_addr_t broad_addr = {255, 255, 255, 255, 255, 255};


void deveth_printhex(char* packet, int len)
{ 
  unsigned char *p;
  int i;
  printf("deveth_process():%d bytes\n",len);  
  if (len > 128) len = 128;
  p = packet;
  i = 1;
  while (len--) {
	printf("%02X",*p);
	if ((i % 2) == 0) printf(" "); /*%=modulo*/
	if ((i % 16) == 0) printf("\n"); /*%=modulo*/
	p++;
	i++;
  }
  printf("\n");
}

int deveth_init(char *fdev, int argc, char **argv)
{
    int i, err;
    __u8 *addr;
    char tmpstring[16];
    
    devfd = open(fdev, O_NONBLOCK|O_RDWR);
    if (devfd < 0) {
	printf("ERROR: failed to open eth device %s\n", fdev);
	return -1;
    }
    
    /* read mac of nic */
    if (ioctl (devfd, IOCTL_ETH_ADDR_GET, eth_local_addr) < 0) {
        perror ("ioctl /dev/eth addr_get");

        /* MAC not available is a fatal error */
        /* because it means the driver does not work */

        return -2;
    }    

    /*
    addr = (__u8 *) &eth_local_addr;
    printf ("eth_local_addr: %2X.%2X.%2X.%2X.%2X.%2X \n",
        addr [0], addr [1], addr [2], addr [3], addr [4],addr [5]);
    */
    
    return devfd;
}


/*
 *  Called when select in ktcp indicates we have new data waiting 
 */

void deveth_process ()
{
  int len;
  int head_size;
  eth_head_t * eth_head;

  head_size = sizeof (eth_head_t);

  len = read (devfd, sbuf, MAX_PACKET_ETH);
  if (len < head_size) return;

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
	  ip_recvpacket (sbuf + head_size, len - head_size);  /* strip link layer */
	  break;

  case ETH_TYPE_ARP:
	  arp_proc (sbuf, len);
	  break;
  }
}


void deveth_send(char *packet, int len)
{ 
    int i;
    //printf("deveth_send:\n");
    //deveth_printhex(packet,len);  
    i = write(devfd, packet, len);
}
