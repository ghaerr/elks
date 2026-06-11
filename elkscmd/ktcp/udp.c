#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "config.h"
#include "ip.h"
#include "udp.h"
#include "dhcp.h"

void udp_send(ipaddr_t dst, __u16 dstport, __u16 srcport,
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
}

void udp_process(struct iphdr_s *iph, unsigned char *packet)
{
    struct udphdr_s *udp = (struct udphdr_s *)packet;
    int udplen = ntohs(udp->len);
    unsigned char *data = packet + sizeof(struct udphdr_s);
    int datalen = udplen - sizeof(struct udphdr_s);

    if (ntohs(udp->dest) == DHCP_CLIENT_PORT) {
	dhcp_input(iph, data, datalen);
    }
}
