#ifndef ARP_H
#define ARP_H

#include "ip.h"


struct arp_addr {
	ipaddr_t daddr;		/* IP destination address */
	ipaddr_t saddr;		/* IP source address */
	__u8  eth_dest[6]; 	/* MAC destination address */
	__u8  eth_src[6]; 	/* MAC source address */
};

struct arp
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

struct arp_cache {
	ipaddr_t   ip_addr;	/* IPv4 address */
	eth_addr_t eth_addr;	/* MAC address */
	int valid;		/* eth_addr valid flag*/
	unsigned char *qpacket;	/* queued packet waiting for ARP reply*/
	int len;		/* queued packet length*/
};

/* arp_cache_get flags*/
#define ARP_VALID	1	/* retrieve valid eth_addr entry only*/
#define ARP_UPDATE	2	/* if present, update cache with passed eth_addr*/

int arp_init (void);
struct arp_cache *arp_cache_get(ipaddr_t ip_addr, eth_addr_t * eth_addr, int flags);
struct arp_cache *arp_cache_update(ipaddr_t ip_addr, eth_addr_t *eth_addr);
struct arp_cache *arp_cache_add(ipaddr_t ip_addr, eth_addr_t * eth_addr);
void arp_recvpacket (unsigned char * packet, int size);
void arp_request(ipaddr_t ipaddress);

char *mac_ntoa(unsigned char *p);

#endif /* !ARP_H */
