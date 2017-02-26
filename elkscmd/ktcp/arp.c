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
#include "ip.h"
#include <linuxmt/arpa/inet.h>
#include "arp.h"



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

int arp_init(void)
{
    return 0;
}

void arp_reply(char *packet,int size)
{
    struct arp_addr apair;
    struct arp *arp_r;
  __u8 *addr;
  
    arp_r = (struct arp *)packet;

    //arp_print(arp_r);

    /* save mac of remote host */
    memcpy(remote_mac, arp_r->eth_src, 6);
    memcpy(acache.remotemac, arp_r->eth_src, 6);
    /* save ip address of remote host */
    acache.ipaddr = arp_r->ip_src;

    /* swap ip addresses and mac addresses */
    apair.daddr = arp_r->ip_src;
    apair.saddr = arp_r->ip_dest;
    memcpy(apair.eth_dest, arp_r->eth_src, 6);
    memcpy(apair.eth_src, local_mac, 6);

    /* build arp reply */
    arp_r->op=0x200; /*response - big endian*/
    memcpy(arp_r->ll_eth_dest, apair.eth_dest, 6);
    memcpy(arp_r->ll_eth_src, apair.eth_src, 6);

    memcpy(arp_r->eth_src, apair.eth_src, 6);
    arp_r->ip_src=apair.saddr;
    memcpy(arp_r->eth_dest, apair.eth_dest, 6);
    arp_r->ip_dest=apair.daddr;

    deveth_send(packet, size);
}

void arp_write_cache(char *packet,int size){
    struct arp *arp_r;
    __u8 *addr;

    arp_r = (struct arp *)packet;
    acache.ipaddr = arp_r->ip_src;  
    memcpy(acache.remotemac, arp_r->eth_src, 6);
/*
    addr = (__u8 *)&acache.ipaddr;
    printf("\nacache.ipaddr: %2X.%2X.%2X.%2X ",addr[0],addr[1],addr[2],addr[3]);
    addr = (__u8 *)&acache.remotemac;
    printf("acache.remotemac: %2X.%2X.%2X.%2X.%2X.%2X \n",addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
*/    
    
    return;
}

int arp_request(ipaddr_t ipaddress)
{
    int i;
    __u8 *addr; 
    fd_set fdset;
    struct arp *arp_r;
    static char packet[sizeof(struct arp)];
    arp_r = (struct arp *)packet;
    
    addr = &ipaddress;
    
    /*clear cache*/
    for (i=0;i<6;i++) acache.remotemac[i]=0x00;
    acache.ipaddr = 0;
    
    /* build arp request */
    for (i=0;i<6;i++) arp_r->ll_eth_dest[i]=0xFF; /*broadcast*/
    memcpy(arp_r->ll_eth_src, local_mac, 6);
    /*specify below in big endian*/
    arp_r->ll_type_len=0x0608;
    arp_r->hard_type=0x0100;
    arp_r->proto_type=0x0008;
    arp_r->hard_len=6;
    arp_r->proto_len=4;
    arp_r->op=0x0100; /*request - big endian*/
    memcpy(arp_r->eth_src, local_mac, 6);
    arp_r->ip_src=local_ip;
    for (i=0;i<6;i++) arp_r->eth_dest[i]=0; 
    arp_r->ip_dest=ipaddress;

    deveth_send(&packet, sizeof(struct arp));

    /* wait for reply */
selectagain:
    FD_ZERO(&fdset);
    FD_SET(tcpdevfd, &fdset);
    i = select(tcpdevfd + 1, &fdset, NULL, NULL, NULL);
    if (i < 0) {
	//if (errno == EINTR) goto selectagain;
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
	switch (arp_r->op)
		{
		case 0x0100:  /* Request big endian */
			arp_reply (packet, size);
			break;

		case 0x0200:  /* Reply big endian */
			arp_write_cache (packet, size);
			break;

		}
	}
