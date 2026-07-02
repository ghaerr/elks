#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "config.h"
#include "ip.h"
#include "udp.h"
#include "dhcp.h"
#include "deveth.h"
#include "timer.h"

int dhcp_enabled;
int dhcp_timer_active;

static int dhcp_state = DHCP_STATE_INIT;
static timeq_t dhcp_retry_time;
static int dhcp_retry_count;
static timeq_t dhcp_start_time;

static __u32 dhcp_xid;
static __u32 dhcp_yiaddr;
static __u32 dhcp_server_ip;
static __u32 dhcp_netmask_val;
static __u32 dhcp_gateway_val;

static unsigned char dhcp_buf[DHCP_MSG_LEN];

static void dhcp_send_discover(void)
{
    struct dhcp_message_s *msg = (struct dhcp_message_s *)dhcp_buf;
    unsigned char *opts;
    int len;

    memset(msg, 0, sizeof(struct dhcp_message_s));
    msg->op = 1;
    msg->htype = 1;
    msg->hlen = 6;
    msg->xid = htonl(dhcp_xid);
    msg->secs = htons((__u16)(((Now - dhcp_start_time) * 60) / 1000));
    msg->flags = htons(0x8000);
    memcpy(msg->chaddr, eth_local_addr, 6);

    opts = (unsigned char *)(msg + 1);
    *(__u32 *)opts = htonl(DHCP_MAGIC_COOKIE);
    opts += 4;

    *opts++ = DHCP_OPT_MSG_TYPE;
    *opts++ = 1;
    *opts++ = DHCP_DISCOVER;

    *opts++ = DHCP_OPT_PARAM_REQ;
    *opts++ = 4;
    *opts++ = DHCP_OPT_SUBNET_MASK;
    *opts++ = DHCP_OPT_ROUTER;
    *opts++ = DHCP_OPT_DNS;
    *opts++ = DHCP_OPT_LEASE_TIME;

    *opts++ = DHCP_OPT_END;

    len = (int)(opts - dhcp_buf);
    udp_send(0xFFFFFFFF, DHCP_SERVER_PORT, DHCP_CLIENT_PORT, dhcp_buf, len, 0);
}

static void dhcp_send_request(void)
{
    struct dhcp_message_s *msg = (struct dhcp_message_s *)dhcp_buf;
    unsigned char *opts;
    int len;

    memset(msg, 0, sizeof(struct dhcp_message_s));
    msg->op = 1;
    msg->htype = 1;
    msg->hlen = 6;
    msg->xid = htonl(dhcp_xid);
    msg->secs = htons((__u16)(((Now - dhcp_start_time) * 60) / 1000));
    msg->flags = htons(0x8000);
    memcpy(msg->chaddr, eth_local_addr, 6);

    opts = (unsigned char *)(msg + 1);
    *(__u32 *)opts = htonl(DHCP_MAGIC_COOKIE);
    opts += 4;

    *opts++ = DHCP_OPT_MSG_TYPE;
    *opts++ = 1;
    *opts++ = DHCP_REQUEST;

    *opts++ = DHCP_OPT_REQ_IP;
    *opts++ = 4;
    *(__u32 *)opts = dhcp_yiaddr;
    opts += 4;

    *opts++ = DHCP_OPT_SERVER_ID;
    *opts++ = 4;
    *(__u32 *)opts = dhcp_server_ip;
    opts += 4;

    *opts++ = DHCP_OPT_END;

    len = (int)(opts - dhcp_buf);
    udp_send(0xFFFFFFFF, DHCP_SERVER_PORT, DHCP_CLIENT_PORT, dhcp_buf, len, 0);
}

static void dhcp_set_retry(void)
{
    timeq_t timeout = DHCP_INIT_TIMEOUT;

    if (dhcp_retry_count > 2)
	timeout = DHCP_MAX_TIMEOUT;
    else {
	int i;
	for (i = 0; i < dhcp_retry_count; i++)
	    timeout *= 2;
	if (timeout > DHCP_MAX_TIMEOUT)
	    timeout = DHCP_MAX_TIMEOUT;
    }
    dhcp_retry_time = Now + timeout;
    dhcp_timer_active = 1;
}

static int dhcp_parse_options(unsigned char *opts, int optlen)
{
    int type, len;

    while (optlen > 0) {
	type = *opts++;
	optlen--;
	if (type == DHCP_OPT_END)
	    break;
	if (type == 0)
	    continue;
	if (optlen < 1)
	    break;
	len = *opts++;
	optlen--;
	if (optlen < len)
	    break;
	switch (type) {
	case DHCP_OPT_MSG_TYPE:
	    if (len >= 1)
		return opts[0];
	    break;
	}
	opts += len;
	optlen -= len;
    }
    return -1;
}

static void dhcp_parse_options_full(unsigned char *opts, int optlen)
{
    int type, len;

    while (optlen > 0) {
	type = *opts++;
	optlen--;
	if (type == DHCP_OPT_END)
	    break;
	if (type == 0)
	    continue;
	if (optlen < 1)
	    break;
	len = *opts++;
	optlen--;
	if (optlen < len)
	    break;
	switch (type) {
	case DHCP_OPT_SUBNET_MASK:
	    if (len >= 4)
		memcpy(&dhcp_netmask_val, opts, 4);
	    break;
	case DHCP_OPT_ROUTER:
	    if (len >= 4)
		memcpy(&dhcp_gateway_val, opts, 4);
	    break;
	case DHCP_OPT_DNS:
	    break;
	case DHCP_OPT_SERVER_ID:
	    if (len >= 4)
		memcpy(&dhcp_server_ip, opts, 4);
	    break;
	case DHCP_OPT_LEASE_TIME:
	    break;
	}
	opts += len;
	optlen -= len;
    }
}

void dhcp_init(void)
{
    if (!dhcp_enabled)
	return;

    dhcp_state = DHCP_STATE_DISCOVER;
    dhcp_retry_count = 0;
    dhcp_start_time = Now;

    dhcp_xid = ((__u32)eth_local_addr[0] << 24) |
	       ((__u32)eth_local_addr[1] << 16) |
	       ((__u32)eth_local_addr[2] << 8)  |
	       eth_local_addr[3];
    dhcp_xid ^= (Now & 0xFF) << 8;

    udp_register(DHCP_CLIENT_PORT, dhcp_input);

    dhcp_send_discover();
    dhcp_set_retry();

    printf("dhcp: sending DISCOVER\n");
}

void dhcp_timer(void)
{
    if (!dhcp_timer_active)
	return;
    if (dhcp_state == DHCP_STATE_BOUND) {
	dhcp_timer_active = 0;
	return;
    }
    if (TIME_GEQ(Now, dhcp_retry_time)) {
	dhcp_retry_count++;
	if (dhcp_retry_count > DHCP_MAX_RETRIES) {
	    printf("dhcp: timeout, restarting\n");
	    dhcp_state = DHCP_STATE_INIT;
	    dhcp_retry_count = 0;
	    dhcp_start_time = Now;
	    dhcp_xid++;
	    dhcp_send_discover();
	    dhcp_set_retry();
	    return;
	}
	switch (dhcp_state) {
	case DHCP_STATE_DISCOVER:
	    printf("dhcp: retry DISCOVER\n");
	    dhcp_send_discover();
	    break;
	case DHCP_STATE_REQUESTING:
	    printf("dhcp: retry REQUEST\n");
	    dhcp_send_request();
	    break;
	}
	dhcp_set_retry();
    }
}

/*
 * UDP callback on port 68 (DHCP_CLIENT_PORT).
 * Handles OFFER, ACK, and NAK replies from the DHCP server.
 * Registered in dhcp_init() via udp_register().
 */
void dhcp_input(struct iphdr_s *iph, uint16_t src_port, unsigned char *data, int len)
{
    struct dhcp_message_s *msg = (struct dhcp_message_s *)data;
    unsigned char *opts;
    int optlen;
    int msg_type;

    /* Minimum DHCP message = fixed header + magic cookie + msg-type option (3 bytes) */

    if (len < (int)sizeof(struct dhcp_message_s) + 4)
	return;
    if (msg->op != 2)			/* 1=request, 2=reply */
	return;
    if (ntohl(msg->xid) != dhcp_xid)	/* must match our transaction ID */
	return;

    opts = (unsigned char *)(msg + 1);
    optlen = len - (int)(opts - data);

    if (optlen < 4)
	return;
    if (*(__u32 *)opts != htonl(DHCP_MAGIC_COOKIE))	/* DHCP option marker */
	return;
    opts += 4;
    optlen -= 4;

    msg_type = dhcp_parse_options(opts, optlen);
    if (msg_type < 0)
	return;

    switch (dhcp_state) {
    case DHCP_STATE_DISCOVER:
	/* Expecting OFFER — save offered IP and send REQUEST */
	if (msg_type == DHCP_OFFER) {
	    dhcp_yiaddr = msg->yiaddr;
	    dhcp_parse_options_full(opts, optlen);
	    printf("dhcp: received OFFER from ");
	    printf("%s", in_ntoa(iph->saddr));
	    printf(", ip %s\n", in_ntoa(dhcp_yiaddr));
	    dhcp_state = DHCP_STATE_REQUESTING;
	    dhcp_retry_count = 0;
	    dhcp_send_request();
	    dhcp_set_retry();
	    printf("dhcp: sending REQUEST\n");
	}
	break;

    case DHCP_STATE_REQUESTING:
	/* Expecting ACK — apply lease (IP, netmask, gateway) */
	if (msg_type == DHCP_ACK) {
	    dhcp_parse_options_full(opts, optlen);
	    if (dhcp_server_ip == 0)
		dhcp_server_ip = iph->saddr;
	    printf("dhcp: received ACK, ip ");
	    printf("%s", in_ntoa(msg->yiaddr));
	    printf(", gateway ");
	    printf("%s", in_ntoa(dhcp_gateway_val));
	    printf(", netmask %s\n", in_ntoa(dhcp_netmask_val));
	    local_ip = msg->yiaddr;
	    if (dhcp_netmask_val)
		netmask_ip = dhcp_netmask_val;
	    if (dhcp_gateway_val)
		gateway_ip = dhcp_gateway_val;
	    dhcp_state = DHCP_STATE_BOUND;
	    dhcp_timer_active = 0;		/* stop retry timer */
	} else if (msg_type == DHCP_NAK) {
	    /* NAK — start over with a new transaction */
	    printf("dhcp: received NAK, restarting\n");
	    dhcp_state = DHCP_STATE_INIT;
	    dhcp_retry_count = 0;
	    dhcp_start_time = Now;
	    dhcp_xid++;
	    dhcp_send_discover();
	    dhcp_set_retry();
	}
	break;
    }
}
