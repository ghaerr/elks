/*
 * Internal UDP implementation for DHCP. Not accessible through sockets.
 */
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "config.h"
#include "ip.h"
#include "icmp.h"
#include "udp.h"
#include "netconf.h"

struct udp_sock udp_socks[MAX_UDP_SOCKS];

int udp_register(uint16_t local_port, udp_callback_t cb)
{
    int i;

    for (i = 0; i < MAX_UDP_SOCKS; i++) {
	if (!udp_socks[i].active) {
	    udp_socks[i].active = 1;
	    udp_socks[i].local_port = local_port;
	    udp_socks[i].remaddr = 0;
	    udp_socks[i].remport = 0;
	    udp_socks[i].callback = cb;
	    return 1;
	}
    }
    return 0;
}

void udp_unregister(uint16_t local_port)
{
    int i;

    for (i = 0; i < MAX_UDP_SOCKS; i++) {
	if (udp_socks[i].active && udp_socks[i].local_port == local_port) {
	    udp_socks[i].active = 0;
	    return;
	}
    }
}

void udp_send(ipaddr_t dst, uint16_t dstport, uint16_t srcport,
	      unsigned char *data, int datalen, ipaddr_t src)
{
    unsigned char buf[sizeof(struct udphdr_s) + 576];
    struct udphdr_s *udp = (struct udphdr_s *)buf;
    struct addr_pair apair;
    int total = sizeof(struct udphdr_s) + datalen;

    udp->src = htons(srcport);
    udp->dest = htons(dstport);
    udp->len = htons(total);
    udp->check = 0;

    memcpy(buf + sizeof(struct udphdr_s), data, datalen);

    apair.daddr = dst;
    apair.saddr = src;
    apair.protocol = PROTO_UDP;
    ip_sendpacket(buf, total, &apair, NULL);
    netstats.udpsndcnt++;
}

void udp_process(struct iphdr_s *iph, unsigned char *packet)
{
    struct udphdr_s *udp = (struct udphdr_s *)packet;
    int udplen = ntohs(udp->len);
    uint16_t dest = ntohs(udp->dest);
    uint16_t src = ntohs(udp->src);
    unsigned char *data = packet + sizeof(struct udphdr_s);
    int datalen = udplen - sizeof(struct udphdr_s);
    int i;

    for (i = 0; i < MAX_UDP_SOCKS; i++) {
	if (udp_socks[i].active && udp_socks[i].local_port == dest) {
	    udp_socks[i].callback(iph, src, data, datalen);
	    netstats.udprcvcnt++;
	    return;
	}
    }
    netstats.udpnoportcnt++;
    icmp_send_port_unreachable(iph);
}
