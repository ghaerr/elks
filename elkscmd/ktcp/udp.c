/*
 * UDP for ktcp.
 *
 * Originally a four-entry callback table serving DHCP only, demultiplexing on
 * the destination port alone and never validating a length. It now backs
 * SOCK_DGRAM sockets: address+port demultiplexing with BSD precedence, a
 * bounded per-socket receive queue, and ICMP error delivery - while keeping
 * udp_register()/udp_unregister() so dhcp.c did not have to change.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include "config.h"
#include "ip.h"
#include "icmp.h"
#include "udp.h"
#include <linuxmt/tcpdev.h>
#include "tcpdev.h"
#include "netconf.h"

struct udp_sock udp_socks[MAX_UDP_SOCKS];

static __u16 udp_next_port = 1024;
static int udp_rcvq_memory;             /* bytes queued across all sockets */

static struct udp_sock *udp_alloc(void)
{
    struct udp_sock *us;

    for (us = udp_socks; us < &udp_socks[MAX_UDP_SOCKS]; us++) {
        if (!us->active) {
            memset(us, 0, sizeof(*us));
            us->active = 1;
            return us;
        }
    }
    return NULL;
}

static struct udp_sock *udp_find_by_port(__u16 port)
{
    struct udp_sock *us;

    for (us = udp_socks; us < &udp_socks[MAX_UDP_SOCKS]; us++)
        if (us->active && us->localport == port)
            return us;
    return NULL;
}

/*
 * Demultiplex, most specific first: a connected socket (both remote address
 * and port set) wins outright, otherwise the first wildcard bound to the port
 * takes it. Standard BSD precedence, in one pass.
 *
 * localaddr 0 means INADDR_ANY, which also makes loopback work: ip_route()
 * loops 127.x internally and the arriving daddr is 127.0.0.1.
 */
static struct udp_sock *udp_lookup(ipaddr_t daddr, __u16 dport,
                                   ipaddr_t saddr, __u16 sport)
{
    struct udp_sock *us, *wild = NULL;

    for (us = udp_socks; us < &udp_socks[MAX_UDP_SOCKS]; us++) {
        if (!us->active || us->localport != dport)
            continue;
        if (us->localaddr && us->localaddr != daddr)
            continue;
        if (us->remport) {                      /* connected */
            if (us->remport == sport && us->remaddr == saddr)
                return us;
            continue;
        }
        if (!wild)
            wild = us;
    }
    return wild;
}

/* ---- internal (callback) users: DHCP ------------------------------------ */

int udp_register(__u16 local_port, udp_callback_t cb)
{
    struct udp_sock *us = udp_alloc();

    if (!us)
        return 0;
    us->localport = local_port;
    us->callback = cb;
    return 1;
}

void udp_unregister(__u16 local_port)
{
    struct udp_sock *us = udp_find_by_port(local_port);

    if (us && us->callback)
        us->active = 0;
}

/* ---- socket-backed users ------------------------------------------------ */

struct udp_sock *udp_sock_find(void *sock)
{
    struct udp_sock *us;

    for (us = udp_socks; us < &udp_socks[MAX_UDP_SOCKS]; us++)
        if (us->active && us->sock == sock)
            return us;
    return NULL;
}

/*
 * Bind. *port is the requested port in host order, 0 to allocate. The
 * ephemeral range is kept separate from TCP's, and the upper bound closes the
 * wrap hole tcpdev_bind() has (a __u16 rolling 65535->0 escapes a "< 1024"
 * guard on every increment but one).
 */
struct udp_sock *udp_sock_new(void *sock, ipaddr_t localaddr, __u16 *port)
{
    struct udp_sock *us;

    if (*port) {
        if (udp_find_by_port(*port))
            return NULL;                        /* -EADDRINUSE */
    } else {
        int tries = 0;
        do {
            if (++udp_next_port < 1024 || udp_next_port > 60000)
                udp_next_port = 1024;
            /* at most MAX_UDP_SOCKS ports are taken, so one free candidate
             * must appear within that many consecutive probes. (int is 16-bit
             * here, so a large bound would never be reached.) */
            if (++tries > MAX_UDP_SOCKS + 1)
                return NULL;
        } while (udp_find_by_port(udp_next_port));
        *port = udp_next_port;
    }

    if (!(us = udp_alloc()))
        return NULL;
    us->sock = sock;
    us->localaddr = localaddr;
    us->localport = *port;
    return us;
}

static void udp_flush(struct udp_sock *us)
{
    struct udp_dgram *d, *next;

    for (d = us->rcvq; d; d = next) {
        next = d->next;
        udp_rcvq_memory -= sizeof(struct udp_dgram) + d->len;
        free(d);
    }
    us->rcvq = NULL;
    us->rcvcount = 0;
}

void udp_sock_free(void *sock)
{
    struct udp_sock *us = udp_sock_find(sock);

    if (us) {
        udp_flush(us);
        us->active = 0;
    }
}

int udp_sock_avail(struct udp_sock *us)
{
    if (us->pending_err)
        return -1;                              /* wakes a blocked reader */
    return us->rcvq? us->rcvq->len: 0;
}

int udp_sock_sendto(struct udp_sock *us, ipaddr_t daddr, __u16 dport,
                    unsigned char *data, int len)
{
    /* ktcp does no IP fragmentation, so refuse rather than truncate */
    if (len + (int)sizeof(struct udphdr_s) > (int)MTU - (int)sizeof(struct iphdr_s))
        return -EMSGSIZE;

    if (us->pending_err) {                      /* report a queued ICMP error */
        int err = us->pending_err;
        us->pending_err = 0;
        return err;
    }

    udp_send(daddr, dport, us->localport, data, len,
             us->localaddr? us->localaddr: local_ip);
    return len;
}

int udp_sock_recvfrom(struct udp_sock *us, unsigned char *buf, int maxlen,
                      ipaddr_t *saddr, __u16 *sport, int *trunc, int *avail_next)
{
    struct udp_dgram *d;
    int n;

    if (us->pending_err) {
        int err = us->pending_err;
        us->pending_err = 0;
        return err;
    }
    if (!(d = us->rcvq))
        return -EAGAIN;

    n = d->len;
    *trunc = (n > maxlen);
    if (n > maxlen)
        n = maxlen;
    memcpy(buf, d->data, n);
    *saddr = d->saddr;
    *sport = d->sport;

    /* a datagram is always consumed whole, never partially */
    us->rcvq = d->next;
    us->rcvcount--;
    udp_rcvq_memory -= sizeof(struct udp_dgram) + d->len;
    free(d);

    *avail_next = us->rcvq? us->rcvq->len: 0;
    return n;
}

static void udp_queue(struct udp_sock *us, ipaddr_t saddr, __u16 sport,
                      unsigned char *data, int len)
{
    struct udp_dgram *d, *tail;

    /*
     * Tail-drop the newest. Dropping the head instead would let a flood starve
     * an application that is making forward progress. No ICMP is sent: the
     * port IS bound, so RFC 1122 does not want a port-unreachable here.
     */
    if (us->rcvcount >= UDP_RCVQ_MAX ||
        udp_rcvq_memory + (int)sizeof(struct udp_dgram) + len > UDP_RCVQ_MAXMEM) {
        netstats.udpdropcnt++;
        return;
    }
    if (!(d = (struct udp_dgram *)malloc(sizeof(struct udp_dgram) + len))) {
        netstats.udpdropcnt++;
        return;
    }
    d->next = NULL;
    d->saddr = saddr;
    d->sport = sport;
    d->len = len;
    memcpy(d->data, data, len);

    if (!us->rcvq)
        us->rcvq = d;
    else {
        for (tail = us->rcvq; tail->next; tail = tail->next)
            ;
        tail->next = d;
    }
    us->rcvcount++;
    udp_rcvq_memory += sizeof(struct udp_dgram) + len;
}

/*
 * ICMP destination-unreachable for a datagram we sent. Only a connected socket
 * can be blamed - for an unconnected one the error cannot be attributed to any
 * particular peer, which is what BSD does too.
 */
void udp_icmp_error(ipaddr_t laddr, __u16 lport, __u16 rport, int err)
{
    struct udp_sock *us;

    for (us = udp_socks; us < &udp_socks[MAX_UDP_SOCKS]; us++) {
        if (us->active && us->sock && us->localport == lport &&
            us->remport == rport && us->remaddr == laddr) {
            us->pending_err = (__s8)err;
            notify_sock(us->sock, TDT_AVAIL_DATA, -1);  /* break a blocked read */
            return;
        }
    }
}

/* ---- wire ---------------------------------------------------------------- */

void udp_send(ipaddr_t dst, __u16 dstport, __u16 srcport,
              unsigned char *data, int datalen, ipaddr_t src)
{
    static unsigned char buf[sizeof(struct udphdr_s) + UDP_MAXPAYLOAD];
    struct udphdr_s *udp = (struct udphdr_s *)buf;
    struct addr_pair apair;
    int total;

    if (datalen < 0 || datalen > UDP_MAXPAYLOAD) {  /* was unchecked */
        netstats.udpdropcnt++;
        return;
    }
    total = sizeof(struct udphdr_s) + datalen;

    udp->src = htons(srcport);
    udp->dest = htons(dstport);
    udp->len = htons(total);
    udp->check = 0;

    memcpy(buf + sizeof(struct udphdr_s), data, datalen);

#if UDP_CHECKSUM
    {
        __u16 sum = ip_chksum_pseudo(buf, src, dst, total, PROTO_UDP);
        udp->check = sum? sum: 0xFFFF;  /* RFC 768: 0 means "none" on the wire */
    }
#endif

    apair.daddr = dst;
    apair.saddr = src;
    apair.protocol = PROTO_UDP;
    ip_sendpacket(buf, total, &apair, NULL);
    netstats.udpsndcnt++;
}

void udp_process(struct iphdr_s *iph, unsigned char *packet, int iplen)
{
    struct udphdr_s *udp = (struct udphdr_s *)packet;
    struct udp_sock *us;
    unsigned int udplen;
    __u16 dest, src;
    unsigned char *data;
    int datalen;

    /*
     * Validate the UDP length against the IP payload we actually received.
     * This was never checked: a peer claiming a short or oversized length
     * produced a negative datalen that flowed straight into memcpy.
     */
    udplen = ntohs(udp->len);
    if (udplen < sizeof(struct udphdr_s) || udplen > (unsigned int)iplen) {
        netstats.ipbadhdr++;
        return;
    }

#if UDP_CHECKSUM
    /* check == 0 means the sender generated none; that is legal, accept it */
    if (udp->check &&
        ip_chksum_pseudo(packet, iph->saddr, iph->daddr, udplen, PROTO_UDP)) {
        netstats.udpbadchksum++;
        return;                         /* silent drop, RFC 1122 - no ICMP */
    }
#endif

    dest = ntohs(udp->dest);
    src = ntohs(udp->src);
    data = packet + sizeof(struct udphdr_s);
    datalen = udplen - sizeof(struct udphdr_s);

    us = udp_lookup(iph->daddr, dest, iph->saddr, src);
    if (!us) {
        netstats.udpnoportcnt++;
        icmp_send_port_unreachable(iph);
        return;
    }
    netstats.udprcvcnt++;

    if (us->callback) {                 /* internal user, e.g. DHCP */
        us->callback(iph, src, data, datalen);
        return;
    }

    udp_queue(us, iph->saddr, src, data, datalen);
    if (us->sock)
        notify_sock(us->sock, TDT_AVAIL_DATA, udp_sock_avail(us));
}
