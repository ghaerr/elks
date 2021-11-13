#ifndef DEVETH_H
#define DEVETH_H


/* Ethernet II types */
/* as big endian values */ //FIXME

#define ETH_TYPE_IPV4 0x0008
#define ETH_TYPE_ARP  0x0608

#define ETH_MTU		1500

/* Ethernet header (no VLAN tag) */

typedef __u8 eth_addr_t [6];

struct eth_head_s {
	eth_addr_t eth_dst;  /* MAC destination address */
	eth_addr_t eth_src;  /* MAC source address */
	__u16 eth_type;      /* Ethernet II type */
};

typedef struct eth_head_s eth_head_t;

struct ip_ll {			//FIXME rename or combine w/above
	__u8  ll_eth_dest[6]; 	/* link layer MAC destination address */
	__u8  ll_eth_src[6]; 	/* link layer MAC source address */
	__u16 ll_type_len;
};

extern eth_addr_t eth_local_addr;

int  deveth_init(char *fdev);
void eth_printhex(unsigned char *packet, int len);
void eth_process(void);
void eth_route(unsigned char *packet, int len, ipaddr_t ip_addr);
void eth_sendpacket(unsigned char *packet, int len, eth_addr_t eth_addr);
void eth_write(unsigned char *packet, int len);

#endif /* !DEVETH_H */
