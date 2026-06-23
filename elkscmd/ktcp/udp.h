#ifndef UDP_H
#define UDP_H

#include <stdint.h>

#define PROTO_UDP	0x11
#define MAX_UDP_SOCKS	4

struct udphdr_s {
	uint16_t	src;
	uint16_t	dest;
	uint16_t	len;
	uint16_t	check;
};

typedef void (*udp_callback_t)(struct iphdr_s *iph, uint16_t src_port,
			       unsigned char *data, int datalen);

struct udp_sock {
    int active;
    uint16_t local_port;
    ipaddr_t remaddr;		/* 0 if not connected */
    uint16_t remport;		/* 0 if not connected */
    udp_callback_t callback;
};

extern struct udp_sock udp_socks[MAX_UDP_SOCKS];

void udp_process(struct iphdr_s *iph, unsigned char *packet);
void udp_send(ipaddr_t dst, uint16_t dstport, uint16_t srcport,
	      unsigned char *data, int datalen, ipaddr_t src);

int udp_register(uint16_t local_port, udp_callback_t cb);
void udp_unregister(uint16_t local_port);

#endif
