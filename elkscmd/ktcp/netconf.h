#ifndef NETCONF_H
#define NETCONF_H

#include "ip.h"

#define NETCONF_PORT	2

struct general_stats_s {
	__u16	retrans_memory;	/* Memory used for retransmition buffers */
	__u8	cb_num; 	/* Number of control blocks */
};

struct cb_stats_s {
	__u32	remaddr;
	__u32	time_wait_exp;
	__u16	rtt;		/* Round trip time in ms */
	__u16	remport;
	__u16	localport;
	__u8	valid;
	__u8	state;
};

struct stat_request_s {
	__u8	type;
	__u8	extra;
};

struct packet_stats_s {
	__u32	ipbadhdr;
	__u32	ipbadchksum;
	__u32	iprcvcnt;
	__u32	ipsndcnt;
	__u32	icmprcvcnt;
	__u32	icmpsndcnt;

	__u32	tcpbadchksum;
	__u32	tcprcvcnt;
	__u32	tcpsndcnt;
	__u32	tcpdropcnt;	/* packet refused or dropped for no space*/
	__u32	tcpretranscnt;

	__u32	ethsndcnt;
	__u32	ethrcvcnt;
	__u32	arprcvreplycnt;
	__u32	arprcvreqcnt;
	__u32	arpsndreplycnt;
	__u32	arpsndreqcnt;
	__u32	arpcacheadds;

	__u32	slipsndcnt;
	__u32	sliprcvcnt;
};

extern struct packet_stats_s netstats;
extern struct tcpcb_s *pending_icmp_cb;	/* netconf client awaiting ICMP echo reply */

#define NS_IDLE 	0
#define NS_GENERAL	1
#define NS_CB		2
#define NS_NETSTATS	3
#define NS_ARP		4
#define NS_ICMP_ECHO	5
#define NS_GET_CONFIG	6
#define NS_SET_IP	7
#define NS_SET_NETMASK	8
#define NS_SET_GATEWAY	9

struct config_info_s {
	ipaddr_t	local_ip;
	ipaddr_t	netmask_ip;
	ipaddr_t	gateway_ip;
	__u8		hwaddr[6];
};

struct icmp_echo_request_s {
	__u32	target_ip;
	__u16	id;
	__u16	seq;
	__u32	timestamp;	/* echo payload timestamp (network order) */
};

struct icmp_echo_reply_s {
	__u32	timestamp;	/* echo payload timestamp (network order) */
	__u8	ttl;		/* TTL from reply IP header */
	__u8	status;		/* 0=success, 1=fail */
};

#define ICMP_ECHO_REPLY_SUCCESS	0
#define ICMP_ECHO_REPLY_FAIL	1

void netconf_init(void);
void netconf_send(struct tcpcb_s *cb);
void netconf_request(struct stat_request_s *sr);

/* forward ICMP echo reply to pending netconf client */
void netconf_icmp_reply(struct tcpcb_s *cb, __u32 timestamp, __u8 ttl);

/* save extra request data for ICMP echo */
void netconf_set_extra(unsigned char *data, int len);

#endif
