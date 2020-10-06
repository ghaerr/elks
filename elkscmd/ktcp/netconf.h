#ifndef NETCONF_H
#define NETCONF_H

#define NETCONF_PORT	2

struct general_stats_s {
	__u16	retrans_memory;	/* Memory used for retransmition buffers */
	__u8	cb_num; 	/* Number of control blocks */
};

struct cb_stats_s {
	__u32	remaddr;
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

#define NS_IDLE 	0
#define NS_GENERAL	1
#define NS_CB		2
#define NS_NETSTATS	3
#define NS_ARP		4

void netconf_init(void);
void netconf_send(struct tcpcb_s *cb);
void netconf_request(struct stat_request_s *sr);

#endif
