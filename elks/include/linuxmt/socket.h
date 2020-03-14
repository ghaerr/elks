#ifndef __LINUXMT_SOCKET_H
#define __LINUXMT_SOCKET_H

#include <stddef.h>
#include <linuxmt/types.h>
#include <linuxmt/uio.h>

#define MAX_SOCK_ADDR 110  /* Sufficient size for AF_UNIX */

struct sockaddr {
    unsigned short sa_family;
    char sa_data [MAX_SOCK_ADDR];
};

struct msghdr {
    void *		msg_name;
    int 		msg_namelen;
    struct iovec *	msg_iov;
    int 		msg_iovlen;
    void *		msg_control;
    int 		msg_controllen;
    int 		msg_flags;
};

#define AF_INET	0		/* Only implemented type */
#define AF_UNIX	1
#define AF_NANO	2

#define PF_INET	AF_INET
#define PF_UNIX	AF_UNIX
#define PF_NANO	AF_NANO

#define SOCK_STREAM     1	/* stream (connection) socket   */
#define SOCK_DGRAM      2	/* datagram (conn.less) socket  */
#define SOCK_RAW        3	/* raw socket                   */
#define SOCK_RDM        4	/* reliably-delivered message   */
#define SOCK_SEQPACKET  5	/* sequential packet socket     */

struct proto_ops;
extern int sock_register(int,struct proto_ops *);
extern int move_addr_to_user(char *,size_t,char *,int *);

#endif
