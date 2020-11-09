#ifndef IP_H
#define IP_H

#define IPOPT_END       0
#define IPOPT_NOOP      1 
#define IPOPT_SEC       130
#define IPOPT_LSRR      131
#define IPOPT_SSRR      137
#define IPOPT_RR        7
#define IPOPT_SID       136
#define IPOPT_TIMESTAMP 68

#define IP_VERSION(s)	((s)->version_ihl >> 4 & 0xf)
#define IP_HLEN(s)	((s)->version_ihl & 0xf)
#define IP_FLAGS(s)	((s)->frag_off >> 13)

typedef __u32	ipaddr_t;

struct addr_pair {
	ipaddr_t daddr;
	ipaddr_t saddr;
	__u8	protocol;
};

struct iphdr_s {
	__u8	version_ihl;
	__u8	tos;
	__u16	tot_len;
	__u16	id;
	__u16	frag_off;
	__u8	ttl;
	__u8	protocol;
	__u16	check;
	ipaddr_t	saddr;
	ipaddr_t	daddr;
};

typedef struct iphdr_s iphdr_t;
struct tcpcb_s;		/* defined in tcp.h */

extern ipaddr_t local_ip;
extern ipaddr_t gateway_ip;
extern ipaddr_t netmask_ip;

#define LINK_ETHER	0
#define LINK_SLIP	1
#define LINK_CSLIP	2

extern int linkprotocol;
extern unsigned int MTU;

int ip_init(void);
__u16 ip_calc_chksum(char *data, int len);
void ip_recvpacket(unsigned char *packet, int size);
void ip_sendpacket(unsigned char *packet, int len, struct addr_pair *apair, struct tcpcb_s *cb);
void ip_route(unsigned char *packet, int len, struct addr_pair *apair);

#endif
