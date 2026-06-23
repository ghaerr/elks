#ifndef DHCP_H
#define DHCP_H

#include <stdint.h>

#define DHCP_CLIENT_PORT	68
#define DHCP_SERVER_PORT	67

#define DHCP_MSG_LEN		576
#define DHCP_MAGIC_COOKIE	0x63825363

#define DHCP_STATE_INIT		0
#define DHCP_STATE_DISCOVER	1
#define DHCP_STATE_SELECTING	2
#define DHCP_STATE_REQUESTING	3
#define DHCP_STATE_BOUND	4

#define DHCP_DISCOVER	1
#define DHCP_OFFER	2
#define DHCP_REQUEST	3
#define DHCP_DECLINE	4
#define DHCP_ACK	5
#define DHCP_NAK	6
#define DHCP_RELEASE	7

#define DHCP_OPT_SUBNET_MASK	1
#define DHCP_OPT_ROUTER		3
#define DHCP_OPT_DNS		6
#define DHCP_OPT_HOST_NAME	12
#define DHCP_OPT_REQ_IP		50
#define DHCP_OPT_LEASE_TIME	51
#define DHCP_OPT_MSG_TYPE	53
#define DHCP_OPT_SERVER_ID	54
#define DHCP_OPT_PARAM_REQ	55
#define DHCP_OPT_END		255

#define DHCP_MAX_RETRIES	5
#define DHCP_INIT_TIMEOUT	(4 << 4)
#define DHCP_MAX_TIMEOUT	(16 << 4)

struct dhcp_message_s {
	__u8	op;
	__u8	htype;
	__u8	hlen;
	__u8	hops;
	__u32	xid;
	__u16	secs;
	__u16	flags;
	__u32	ciaddr;
	__u32	yiaddr;
	__u32	siaddr;
	__u32	giaddr;
	__u8	chaddr[16];
	__u8	sname[64];
	__u8	file[128];
};

extern int dhcp_enabled;
extern int dhcp_timer_active;

void dhcp_init(void);
void dhcp_timer(void);
void dhcp_input(struct iphdr_s *iph, uint16_t src_port, unsigned char *data, int len);

#endif
