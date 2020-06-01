#ifndef DEVETH_H
#define DEVETH_H


/* Ethernet II types */
/* as big endian values */ //FIXME

#define ETH_TYPE_IPV4 0x0008
#define ETH_TYPE_ARP  0x0608


/* Ethernet header (no VLAN tag) */

typedef __u8 eth_addr_t [6];

struct eth_head_s {
	eth_addr_t eth_dst;  /* MAC destination address */
	eth_addr_t eth_src;  /* MAC source address */

	__u16 eth_type;      /* Ethernet II type */
	};

typedef struct eth_head_s eth_head_t;

extern eth_addr_t eth_local_addr;
extern int eth_device;

void deveth_printhex(unsigned char *packet, int len);
int  deveth_init(char *fdev);
void deveth_process(int flag);
void deveth_send(unsigned char *packet, int len);

#endif /* !DEVETH_H */
