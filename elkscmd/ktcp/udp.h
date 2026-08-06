#ifndef UDP_H
#define UDP_H

/* was a four entry callback table for dhcp only, now a real demultiplexer
 * backing SOCK_DGRAM. callback form kept so dhcp.c is unchanged */

#define PROTO_UDP       0x11

/* largest datagram payload ktcp will build: DHCP needs 576, the socket path is
 * bounded by TDB_WRITE_MAX (512), so 576 covers both */
#define UDP_MAXPAYLOAD  576

#define MAX_UDP_SOCKS   8       /* internal (DHCP) + socket-backed, machine wide */

/* receive queue limits. a deep queue only turns loss into latency so keep it
 * shallow. the byte cap is the real limit and must hold four full size
 * datagrams, at 2048 a sender emitting two back to back lost one of each
 * pair */
#define UDP_RCVQ_MAX    4       /* datagrams queued per socket */
#define UDP_RCVQ_MAXMEM 6144    /* bytes queued across all UDP sockets */

struct udphdr_s {
    __u16   src;
    __u16   dest;
    __u16   len;
    __u16   check;
};

typedef void (*udp_callback_t)(struct iphdr_s *iph, __u16 src_port,
                               unsigned char *data, int datalen);

/* one queued datagram; header is 10 bytes, payload follows */
struct udp_dgram {
    struct udp_dgram *next;
    ipaddr_t          saddr;    /* network byte order */
    __u16             sport;    /* host byte order */
    __u16             len;
    unsigned char     data[];
};

struct udp_sock {
    void             *sock;     /* kernel socket handle; NULL for internal users */
    udp_callback_t    callback; /* non-NULL for internal users (DHCP) */
    ipaddr_t          localaddr;/* network order; 0 = INADDR_ANY */
    ipaddr_t          remaddr;  /* network order; 0 = unconnected */
    struct udp_dgram *rcvq;     /* FIFO, oldest first */
    __u16             localport;/* host order */
    __u16             remport;  /* host order; 0 = unconnected */
    __u8              active;
    __s8              pending_err; /* negative errno from ICMP, else 0 */
    __u8              rcvcount;
};

extern struct udp_sock udp_socks[MAX_UDP_SOCKS];

void udp_process(struct iphdr_s *iph, unsigned char *packet, int iplen);
void udp_send(ipaddr_t dst, __u16 dstport, __u16 srcport,
              unsigned char *data, int datalen, ipaddr_t src);

/* internal (callback) users - DHCP */
int  udp_register(__u16 local_port, udp_callback_t cb);
void udp_unregister(__u16 local_port);

/* socket-backed users, driven from tcpdev.c */
struct udp_sock *udp_sock_new(void *sock, ipaddr_t localaddr, __u16 *port);
struct udp_sock *udp_sock_find(void *sock);
void udp_sock_free(void *sock);
int  udp_sock_sendto(struct udp_sock *us, ipaddr_t daddr, __u16 dport,
                     unsigned char *data, int len);
int  udp_sock_recvfrom(struct udp_sock *us, unsigned char *buf, int maxlen,
                       ipaddr_t *saddr, __u16 *sport, int *trunc, int *avail_next);
int  udp_sock_avail(struct udp_sock *us);
void udp_icmp_error(ipaddr_t laddr, __u16 lport, __u16 rport, int err);

#endif
