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
#define TCPDEV_OUTBUFFERSIZE	(TDB_WRITE_MAX + sizeof(struct tdb_write))

#define TCPDEV_MAXREAD TCPDEV_INBUFFERSIZE - sizeof(struct tdb_return_data)

/* outgoing ops */
#define TDC_BIND	1
#define TDC_ACCEPT	2
#define TDC_CONNECT	3
#define TDC_LISTEN	4
#define TDC_RELEASE	5
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
    struct socket *sock;
    int reuse_addr;
    int rcv_bufsiz;
    struct sockaddr_in addr;
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
};

struct tdb_bind_ret {
    char type;
    int ret_value;
    struct socket *sock;
    __u32 addr_ip;
    __u16 addr_port;
};

extern void tcpdev_clear_data_avail(void);
extern int inet_process_tcpdev(char *buf, int len);

#endif
