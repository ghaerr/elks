#ifndef LX86_LINUXMT_SOCKET_H
#define LX86_LINUXMT_SOCKET_H

#include <linuxmt/uio.h>

struct sockaddr {
    unsigned short sa_family;
    char sa_data[14];
};

struct msghdr {
    void *msg_name;
    int msg_namelen;
    struct iovec *msg_iov;
    int msg_iovlen;
    void *msg_control;
    int msg_controllen;
    int msg_flags;
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

#endif
