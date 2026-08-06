#ifndef __LINUXMT_SOCKET_H
#define __LINUXMT_SOCKET_H

#include <linuxmt/types.h>

#define MAX_SOCK_ADDR 110  /* Sufficient size for AF_UNIX */

struct sockaddr {
    unsigned short sa_family;
    char sa_data [MAX_SOCK_ADDR];
};

/* for setsockopt(2) */
#define SOL_SOCKET	1

/* careful: option names are close to internal SF_ options in net.h*/
#define SO_REUSEADDR	2
#define SO_RCVBUF	8		/* set TCP CB receive buffer size*/
#define SO_LINGER	13		/* only implemented for l_linger = 0*/

/* non-standard options */
/* SO_RCVBUF on a socket that then listens sets the ring of the connections
 * it accepts, the listener never receives anything itself */
#define SO_LISTEN_BUFSIZ	128	/* deprecated, was "small ring for the listener
					 * itself"; too small to accept data into */
#define SO_ACCEPT_BUFSIZ_TINY	512	/* small fixed packets: telnetd, elkscraft */
#define SO_ACCEPT_BUFSIZ_SMALL	1024	/* request/response: httpd, ftpd control */
#define SO_ACCEPT_BUFSIZ_BULK	4096	/* streaming: ftpd data, audiorecv, vidrecv */

struct linger {
        int             l_onoff;        /* Linger active                */
        int             l_linger;       /* How long to linger for       */
};

/* only the two we can honour are accepted, the rest are -EOPNOTSUPP rather
 * than silently ignored */
#define MSG_OOB		0x01	/* not supported */
#define MSG_PEEK	0x02	/* not supported */
#define MSG_DONTROUTE	0x04	/* not supported */
#define MSG_TRUNC	0x20	/* recv: report the real datagram length */
#define MSG_DONTWAIT	0x40	/* supported */
#define MSG_SUPPORTED	(MSG_TRUNC|MSG_DONTWAIT)

#define AF_INET	0
#define AF_UNIX	1

#define PF_INET	AF_INET
#define PF_UNIX	AF_UNIX

#define AF_LOCAL AF_UNIX
#define PF_LOCAL PF_UNIX

#define SOCK_STREAM     1	/* stream (connection) socket   */
#define SOCK_DGRAM      2	/* datagram (conn.less) socket  */
#define SOCK_RAW        3	/* raw socket                   */
#define SOCK_RDM        4	/* reliably-delivered message   */
#define SOCK_SEQPACKET  5	/* sequential packet socket     */

#ifdef __KERNEL__
struct proto_ops;
struct socket;
int sock_register(int,struct proto_ops *);
int move_addr_to_user(char *,size_t,char *,int *);
int sock_awaitconn(struct socket *mysock, struct socket *servsock, int flags);
#endif

#endif
