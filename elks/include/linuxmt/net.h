#ifndef __LINUXMT_NET_H__
#define __LINUXMT_NET_H__

#include <linuxmt/config.h>
#include <linuxmt/wait.h>
#include <linuxmt/socket.h>

/* Number of protocols */
#define NPROTO 3

#define SOCK_INODE(S)	((S)->inode)

typedef enum
{
    SS_FREE = 0,
    SS_UNCONNECTED,
    SS_CONNECTING,
    SS_CONNECTED,
    SS_DISCONNECTING
}
socket_state;

struct socket
{
    short type;
    unsigned char state;
    long flags;
    struct proto_ops *ops;
    void *data;
#if defined(CONFIG_INET)
    int avail_data;		/* I added this here for smaller code and memory use - HarKal */
    short sem;
#endif
#if defined(CONFIG_UNIX) || defined(CONFIG_NANO) || defined(CONFIG_INET)
    struct socket *conn;
    struct socket *iconn;
    struct socket *next;
#endif
    struct wait_queue *wait;
    struct inode *inode;
    struct fasync_struct *fasync_list;
    struct file *file;
};

struct proto_ops
{
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

#define SO_ACCEPTCON	(1<<13)
#define SO_WAITDATA	(1<<14)
#define SO_NOSPACE	(1<<15)

struct net_proto
{
    char *name;			/* Protocol name */
    void (*init_func) ();	/* Bootstrap */
};

#endif
