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
#define SO_LISTEN_BUFSIZ	128	/* suggested buffer size for listen to save mem*/

struct linger {
        int             l_onoff;        /* Linger active                */
        int             l_linger;       /* How long to linger for       */
};

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
