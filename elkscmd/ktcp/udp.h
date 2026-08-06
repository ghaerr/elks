#ifndef UDP_H
#define UDP_H

/*
 * UDP for ktcp. This was a four-entry callback table that existed only so DHCP
 * could work, with its own header saying "Not accessible through sockets". It
 * is now a real demultiplexer backing SOCK_DGRAM, while keeping the callback
 * form so dhcp.c is unchanged.
 */

#define PROTO_UDP       0x11

/* largest datagram payload ktcp will build: DHCP needs 576, the socket path is
 * bounded by TDB_WRITE_MAX (512), so 576 covers both */
#define UDP_MAXPAYLOAD  576

#define MAX_UDP_SOCKS   8       /* internal (DHCP) + socket-backed, machine wide */

/*
 * Receive queue limits. Datagrams are lost rather than retransmitted, so a
 * deep queue only converts loss into latency - which is the wrong trade for
 * the media receivers this exists for. Four covers a duplicate or late reply
 * plus one scheduling hiccup.
 *
 * The byte cap is the load-bearing one: per-socket counts alone would allow
 * 8 x 4 x 1482 = 47424 bytes, more than ktcp's entire heap.
 *
 * It must be sized in FULL-SIZE datagrams, not in bytes that merely look
 * generous. At 2048 a single 1027-byte datagram plus its 10-byte queue header
 * fitted but a second did not, so any sender emitting two back to back - which
 * is exactly what a video stream with separate audio and picture datagrams
 * does - lost one of every pair before the application ever looked. 6144 holds
 * four 1482-byte datagrams and their headers. That is worst case, reached only
 * while an application is actually behind; against a 49152-byte heap it costs
 * less than three TCP control blocks, and nothing at all when the queue drains.
 */
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
