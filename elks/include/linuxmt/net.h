#ifndef LINUXMT_NET_H
#define LINUXMT_NET_H

#include <linuxmt/config.h>
#include <linuxmt/wait.h>
#include <linuxmt/socket.h>

/* Number of protocols */
#define NPROTO 1

#define SOCK_INODE(S)	((S)->inode)

typedef enum {
	SS_FREE = 0,
	SS_UNCONNECTED,
	SS_CONNECTING,
	SS_CONNECTED,
	SS_DISCONNECTING
} socket_state;

struct socket {
	short			type;
	socket_state		state;
	long			flags;
	struct proto_ops	*ops;
	void			*data;
#if defined(CONFIG_UNIX) || defined(CONFIG_NANO)
	struct socket		*conn;
	struct socket		*iconn;
	struct socket		*next;
#endif /* CONFIG_UNIX || CONFIG_NANO */
	struct wait_queue	*wait;
	struct inode		*inode;
	struct fasync_struct	*fasync_list;
	struct file		*file;
};

struct proto_ops {
	int	family;
	int	(*create)	();
	int	(*dup)		();
	int	(*release)	();
	int	(*bind)		();
	int	(*connect)	();
 	int	(*socketpair)	();
	int	(*accept)	();
	int	(*getname)	();
	int	(*read)		();
	int	(*write)	();
	int	(*select)	();
	int	(*ioctl)	();
	int	(*listen)	();
	int	(*send)		();
	int	(*recv)		();
	int	(*sendto)	();
	int	(*recvfrom)	();
	int	(*shutdown)	();
	int	(*setsocketopt)	();
	int	(*getsocketopt)	();
	int	(*fcntl)	();
};

#define SO_ACCEPTCON	(1<<13)
#define SO_WAITDATA	(1<<14)
#define SO_NOSPACE	(1<<15)

struct net_proto {
	char *name;               /* Protocol name */
	void (*init_func)();  /* Bootstrap */
};


#endif /* LINUXMT_NET_H */
