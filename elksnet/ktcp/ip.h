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

struct addr_pair {
	__u32	daddr;
	__u32	saddr;
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
	__u32	saddr;
	__u32	daddr;
};

long 	local_ip;
__u16	next_port;

#endif
