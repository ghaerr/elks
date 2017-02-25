/*
 * This file is part of the ELKS TCP/IP stack
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "deveth.h"
#include "arp.h"
#include "tcpdev.h"
#include <arch/ioctl.h>

/*#define DEBUG*/

#define MTU 2048

static unsigned char sbuf[TCPDEV_BUFSIZE];
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
void deveth_process(void)
{
  int len;
  struct arp *arp_r;

  len = read(devfd, &sbuf, TCPDEV_BUFSIZE);
  if (len==-1) return;
  
  arp_r = (struct arp *)&sbuf;
  if (arp_r->proto_type == 8) { /* =0x800 big endian */
    if (arp_r->op == 0x0100) { /*Request big endian */
      arp_reply(&sbuf, len);
    } else if (arp_r->op == 0x0200) { /*Reply big endian */
      arp_write_cache(&sbuf, len);
    }
    
  } else {
    memmove(&sbuf,&sbuf[14], TCPDEV_BUFSIZE); /*strip link layer*/
    ip_recvpacket(&sbuf,len); 
  }
}

void deveth_send(char *packet, int len)
{ 
    int i;
    //printf("deveth_send:\n");
    //deveth_printhex(packet,len);  
    i = write(devfd, packet, len);
}



