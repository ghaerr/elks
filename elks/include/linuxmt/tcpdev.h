#ifndef __LINUXMT_TCPDEV_H
#define __LINUXMT_TCPDEV_H

/* tcpdev.h header for ELKS kernel
 * Copyright (C) 2001 Harry Kalogirou
 */

#include <linuxmt/in.h>
#include <linuxmt/net.h>

#define TCP_DEVICE_NAME	"tcpdev"

/* should be equal to PTYOUTQ_SIZE and telnetd buffer size*/
#define	TDB_WRITE_MAX		512	/* max data in tdb_write packet to ktcp*/

#define TCPDEV_INBUFFERSIZE	1500	/* max data writable to tcpdev from ktcp*/

/* largest kernel->ktcp message. tdb_sendto is the bigger of the two as it
 * also carries an address. do not add TDB_WRITE_MAX on top, it is embedded */
#define TCPDEV_OUTBUFFERSIZE	(sizeof(struct tdb_sendto) > sizeof(struct tdb_write)? \
				 sizeof(struct tdb_sendto): sizeof(struct tdb_write))

/* max datagram deliverable up to the kernel in one TDT_RECVFROM */
#define TCPDEV_MAXDGRAM		(TCPDEV_INBUFFERSIZE - sizeof(struct tdb_recvfrom_ret))

#define TCPDEV_MAXREAD		(TCPDEV_INBUFFERSIZE - sizeof(struct tdb_return_data))

/* outgoing ops */
#define TDC_BIND	1
#define TDC_ACCEPT	2
#define TDC_CONNECT	3
#define TDC_LISTEN	4
#define TDC_RELEASE	5
#define TDC_SENDTO	6	/* datagram out, carries the destination */
#define TDC_RECVFROM	7	/* datagram in, reply carries the source */
#define TDC_READ	8
#define TDC_WRITE	9

struct tdb_release {
    unsigned char cmd;
    struct socket *sock;
    int reset;
};

struct tdb_accept {
    unsigned char cmd;
    struct socket *sock;
    struct socket *newsock;
    int nonblock;
};

struct tdb_listen {
    unsigned char cmd;
    struct socket *sock;
    int backlog;
};

struct tdb_bind {
    unsigned char cmd;
    unsigned char proto;	/* 0 = SOCK_STREAM, 1 = SOCK_DGRAM. Uses the pad
				 * byte that was already here for alignment */
    struct socket *sock;
    int reuse_addr;
    int rcv_bufsiz;
    struct sockaddr_in addr;
};

#define TDB_PROTO_STREAM	0
#define TDB_PROTO_DGRAM		1

/* datagram send. atomic, so no chunking loop, oversize is -EMSGSIZE */
struct tdb_sendto {
    unsigned char cmd;
    unsigned char msgflags;	/* MSG_* (was padding) */
    struct socket *sock;
    __u32 daddr;		/* destination, network byte order */
    __u16 dport;		/* destination port, network byte order */
    int size;
    int nonblock;
    unsigned char data[TDB_WRITE_MAX];
};

struct tdb_recvfrom {
    unsigned char cmd;
    unsigned char msgflags;
    struct socket *sock;
    int size;			/* user buffer size */
    int nonblock;
};

struct tdb_connect {
    unsigned char cmd;
    struct socket *sock;
    struct sockaddr_in addr;
};

struct tdb_read {
    unsigned char cmd;
    struct socket *sock;
    int size;
    int nonblock;
};

struct tdb_write {
    unsigned char cmd;
    struct socket *sock;
    int size;
    int nonblock;
    unsigned char data[TDB_WRITE_MAX];
};

/* incoming (ktcp to kernel) ops */
#define	TDT_RETURN	1
#define	TDT_CHG_STATE	2
#define	TDT_AVAIL_DATA	3
#define TDT_ACCEPT	4
#define TDT_BIND	5
#define TDT_CONNECT	6
#define TDT_RECVFROM	7	/* datagram up, with its source address */

struct tdb_return_data {
    char type;
    int ret_value;
    struct socket *sock;
    int size;
    unsigned char data[];
};

struct tdb_accept_ret {
    char type;
    int ret_value;
    struct socket *sock;
    __u32 addr_ip;
    __u16 addr_port;
    __u32 locaddr;
    __u16 locport;
};

struct tdb_bind_ret {
    char type;
    int ret_value;
    struct socket *sock;
    __u32 addr_ip;
    __u16 addr_port;
};

/* first four members match struct tdb_return_data so inet_process_tcpdev
 * can switch on ->type without knowing which reply it has. avail_next is the
 * size of the next queued datagram, ktcp must not send a separate
 * TDT_AVAIL_DATA or it stalls the stack on the shared buffer */
struct tdb_recvfrom_ret {
    char type;			/*  0 */
    char trunc;			/*  1  datagram was longer than the buffer */
    int ret_value;		/*  2  bytes copied, or -errno */
    struct socket *sock;	/*  4 */
    int size;			/*  6  bytes of data[] following */
    __u32 saddr;		/*  8  source, network byte order */
    __u16 sport;		/* 12  source port, network byte order */
    __u16 avail_next;		/* 14  size of next queued datagram, 0 if none */
    unsigned char data[];	/* 16 */
};

#ifdef __KERNEL__
/* kernel-internal; ktcp includes this header too and must not see these */
#include <linuxmt/init.h>
extern void tcpdev_clear_data_avail(void);
extern int FARPROC inet_process_tcpdev(char *buf, int len);
#endif

#endif
