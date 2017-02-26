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


/*#define DEBUG*/

static unsigned char sbuf [MAX_PACKET_ETH];
static int devfd;


void deveth_printhex(char* packet, int len)
{ 
  unsigned char *p;
  int i;
  printf("deveth_process():%d bytes\n",len);  
  if (len>128) len=128;
  p=packet;
  i=1;
  while (len--) {
	printf("%02X",*p);
	if ((i % 2)==0) printf(" "); /*%=modulo*/
	if ((i % 16)==0) printf("\n"); /*%=modulo*/
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
	printf("ERROR : failed to open eth device %s\n",fdev);
	return -1;
    }
    
    /* read mac of nic */
    if (ioctl (devfd, IOCTL_ETH_ADDR_GET, local_mac)<0) {
	perror ("ioctl /dev/eth addr_get local_mac");
	memcpy(local_mac,default_mac, 6);
    }    
    
    //printf("argc:%d,argv[2]:%s\n",argc,argv[2]);

    arp_request(gateway_ip); /*updates ARP cache*/
    memcpy(gateway_mac,acache.remotemac, 6);
    /*clear cache again*/
    for (i=0;i<6;i++) acache.remotemac[i]=0x00;
    acache.ipaddr = 0;
    
    arp_request(nameserver_ip); /*updates ARP cache*/
    memcpy(nameserver_mac,acache.remotemac, 6);
    /*clear cache again*/
    for (i=0;i<6;i++) acache.remotemac[i]=0x00;
    acache.ipaddr = 0;
    if (strlen(nameserver_mac)==0)
       memcpy(nameserver_mac,gateway_mac, 6); /*need to use gateway*/
/*    
    addr = (__u8 *)&local_ip;
    printf("\nlocal_ip: %2X.%2X.%2X.%2X ",addr[0],addr[1],addr[2],addr[3]);    
    addr = (__u8 *)&local_mac;
    printf("local_mac: %2X.%2X.%2X.%2X.%2X.%2X \n",addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
    addr = (__u8 *)&gateway_ip;
    printf("\ngateway_ip: %2X.%2X.%2X.%2X ",addr[0],addr[1],addr[2],addr[3]);
    addr = (__u8 *)&gateway_mac;
    printf("gateway_mac: %2X.%2X.%2X.%2X.%2X.%2X \n",addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
    addr = (__u8 *)&nameserver_ip;
    printf("nameserver_ip: %2X.%2X.%2X.%2X ",addr[0],addr[1],addr[2],addr[3]);
    addr = (__u8 *)&nameserver_mac;
    printf("nameserver_mac: %2X.%2X.%2X.%2X.%2X.%2X \n",addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
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

  /* Decode Ethernet II header */
  /* and dispatch to protocols */

  eth_head = (eth_head_t *) sbuf;
  switch (eth_head->eth_type)
	  {
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
