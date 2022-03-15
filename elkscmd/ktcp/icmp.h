#ifndef ICMP_H
#define ICMP_H

#define PROTO_ICMP	1

#define ICMP_MIN_HDR_LEN	4

#define ICMP_TYPE_ECHO_REPL	0
#define ICMP_TYPE_DST_UNRCH	3
#	define ICMP_NET_UNRCH			0
#	define ICMP_HOST_UNRCH			1
#	define ICMP_PROTOCOL_UNRCH		2
#	define ICMP_PORT_UNRCH			3
#	define ICMP_FRAGM_AND_DF		4
#	define ICMP_SOURCE_ROUTE_FAILED		5
#define ICMP_TYPE_SRC_QUENCH	4
#define ICMP_TYPE_REDIRECT	5
#	define ICMP_REDIRECT_NET		0
#	define ICMP_REDIRECT_HOST		1
#	define ICMP_REDIRECT_TOS_AND_NET	2
#	define ICMP_REDIRECT_TOS_AND_HOST	3
#define ICMP_TYPE_ECHO_REQ	8
#define ICMP_TYPE_ROUTER_ADVER	9
#define ICMP_TYPE_ROUTE_SOL	10
#define ICMP_TYPE_TIME_EXCEEDED	11
#	define ICMP_TTL_EXC			0
#	define ICMP_FRAG_REASSEM		1
#define ICMP_TYPE_PARAM_PROBLEM	12
#define ICMP_TYPE_TS_REQ	13
#define ICMP_TYPE_TS_REPL	14
#define ICMP_TYPE_INFO_REQ	15
#define ICMP_TYPE_INFO_REPL	16

struct icmp_echo_s {
	__u8	type;
	__u8	code;
	__u16	chksum;
	__u16	id;
	__u16	seqnum;
};

struct icmp_dest_unreachable_s {
	__u8	type;
	__u8	code;
	__u16	chksum;
	__u16	unused;
	__u16	nexthop_mtu;
	unsigned char iphdr[];
};

int icmp_init(void);
void icmp_process(struct iphdr_s *iph, unsigned char *packet);

#endif
