#ifndef ARP_H
#define ARP_H

#include "ip.h"


typedef struct arp_addr {
	ipaddr_t daddr;		/* IP destination address */
	ipaddr_t saddr;		/* IP source address */
	__u8  eth_dest[6]; 	/* MAC destination address */
	__u8  eth_src[6]; 	/* MAC source address */
};

typedef struct arp
{
         __u8  ll_eth_dest[6]; 	/* link layer MAC destination address */
         __u8  ll_eth_src[6]; 	/* link layer MAC source address */
         __u16 ll_type_len;
         __u16 hard_type;
         __u16 proto_type;
         __u8  hard_len;
         __u8  proto_len;
         __u16 op; 		/* ARP operation code (command) */
         __u8  eth_src[6]; 	/* MAC source address */
         __u32 ip_src; 		/* IP source address */
         __u8  eth_dest[6]; 	/* MAC destination address */
         __u32 ip_dest; 	/* IP destination address */
};

int arp_init ();

void arp_cache_add (arp_cache_t * pair);
int arp_cache_get (ipaddr_t * ip_addr, eth_addr_t * eth_addr);

void arp_proc (char * packet, int size);


#endif /* !ARP_H */
