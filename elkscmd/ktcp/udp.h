#ifndef UDP_H
#define UDP_H

#define PROTO_UDP       0x11
#define MAX_UDP_SOCKS   4

struct udphdr_s {
    __u16   src;
    __u16   dest;
    __u16   len;
    __u16   check;
};

typedef void (*udp_callback_t)(struct iphdr_s *iph, __u16 src_port,
                               unsigned char *data, int datalen);

struct udp_sock {
    int active;
    __u16 local_port;
    ipaddr_t remaddr;           /* 0 if not connected */
    __u16 remport;              /* 0 if not connected */
    udp_callback_t callback;
};

extern struct udp_sock udp_socks[MAX_UDP_SOCKS];

void udp_process(struct iphdr_s *iph, unsigned char *packet);
void udp_send(ipaddr_t dst, __u16 dstport, __u16 srcport,
              unsigned char *data, int datalen, ipaddr_t src);

int udp_register(__u16 local_port, udp_callback_t cb);
void udp_unregister(__u16 local_port);

#endif
