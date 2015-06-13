#ifndef LX86_LINUXMT_TCPDEV_H
#define LX86_LINUXMT_TCPDEV_H

/* tcpdev.h header for ELKS kernel
 * Copyright (C) 2001 Harry Kalogirou
 */

#include <linuxmt/in.h>
#include <linuxmt/net.h>

#define TCP_DEVICE_NAME	"tcpdev"

#define TCPDEV_INBUFFERSIZE	1024
#define TCPDEV_OUTBUFFERSIZE	128

#define TCPDEV_MAXREAD TCPDEV_INBUFFERSIZE - sizeof(struct tdb_return_data)

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
#define	TDB_WRITE_MAX	100
    unsigned char data[TDB_WRITE_MAX];
};

#define	TDT_RETURN	1
#define	TDT_CHG_STATE	2
#define	TDT_AVAIL_DATA	3

struct tdb_return_data {
    char type;
    int ret_value;
    struct socket *sock;
    int size;
    unsigned char data;
};

struct tdb_accept_ret {
    char type;
    int ret_value;
    struct socket *sock;
    __u32 addr_ip;
    __u16 addr_port;
};

extern void tcpdev_clear_data_avail(void);

#endif
