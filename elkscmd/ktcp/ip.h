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
#define IP_IHL(s)	((s)->version_ihl & 0xf)
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

__u16 ip_calc_chksum(char *data, int len);

ipaddr_t local_ip;
ipaddr_t gateway_ip;
ipaddr_t netmask_ip;
ipaddr_t nameserver_ip;
__u16	next_port;

typedef struct ip_ll
{
         __u8  ll_eth_dest[6]; 	/* link layer MAC destination address */
         __u8  ll_eth_src[6]; 	/* link layer MAC source address */
         __u16 ll_type_len;	/* 0x800 big endian for IP */
};

typedef struct arp_cache {
	ipaddr_t ipaddr;	/* IP remote address */
	__u8  remotemac[6]; 	/* MAC remote address */
};

static int tcpdevfd;

struct arp_cache acache;
static char default_mac [6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};  // QEMU default
static char local_mac [6];
char remote_mac [6];
char gateway_mac [6];
char netmask_mac [6];
char nameserver_mac [6];

#endif
