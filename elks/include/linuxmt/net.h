#ifndef __LINUXMT_NET_H
#define __LINUXMT_NET_H

#include <linuxmt/types.h>
#include <linuxmt/config.h>
#include <linuxmt/socket.h>

/* Number of protocols */
#define NPROTO 3

#define SOCK_INODE(S)	((S)->inode)

typedef enum {
    SS_FREE = 0,
    SS_UNCONNECTED,
    SS_CONNECTING,
    SS_CONNECTED,
    SS_DISCONNECTING
} socket_state;

struct socket {
    unsigned char state;
    struct wait_queue *wait;
    unsigned char flags;
    unsigned int rcv_bufsiz;
    struct proto_ops *ops;
    struct inode *inode;
    struct file *file;

#if defined(CONFIG_UNIX) || defined(CONFIG_NANO)
    struct socket *conn;
    struct socket *iconn;
    struct socket *next;
    void *data;
#endif

#if defined(CONFIG_INET)
    sem_t sem;			/* one operation at a time per socket */
    int avail_data;		/* data available for reading from ktcp */
    int retval;			/* event return value from ktcp */
    __u32 remaddr;		/* all in network byte order */
    __u32 localaddr;
    __u16 remport;
    __u16 localport;
#endif

};

struct proto_ops {
    int family;
    int (*create) ();
    int (*dup) ();
    int (*release) ();
    int (*bind) ();
    int (*connect) ();
    int (*socketpair) ();
    int (*accept) ();
    int (*getname) ();
    int (*read) ();
    int (*write) ();
    int (*select) ();
    int (*ioctl) ();
    int (*listen) ();
    int (*send) ();
    int (*recv) ();
    int (*sendto) ();
    int (*recvfrom) ();
    int (*shutdown) ();
    int (*setsocketopt) ();
    int (*getsocketopt) ();
    int (*fcntl) ();
};

/* careful: option names are close to public SO_ options in socket.h */
#define SF_CLOSING	(1 << 0) /* inet */
#define SF_ACCEPTCON	(1 << 1) /* unix, nano, sockets */
#define SF_WAITDATA	(1 << 2) /* unix, nano */
#define SF_NOSPACE	(1 << 3) /* unix, nano */
#define SF_RST_ON_CLOSE	(1 << 4) /* inet */
#define SF_REUSE_ADDR	(1 << 5) /* inet */
#define SF_CONNECT	(1 << 6) /* inet */

struct net_proto {
    const char *name;		/* Protocol name */
    void (*init_func) ();	/* Bootstrap */
};

#endif
