#ifndef UDP_H
#define UDP_H

#define PROTO_UDP	0x11

struct udphdr_s {
	__u16	src;
	__u16	dest;
	__u16	len;
	__u16	check;
};

void udp_process(struct iphdr_s *iph, unsigned char *packet);
void udp_send(ipaddr_t dst, __u16 dstport, __u16 srcport,
	      unsigned char *data, int datalen, ipaddr_t src);

#endif
