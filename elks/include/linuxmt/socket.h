#ifndef _LINUXMT_SOCKET_H
#define _LINUXMT_SOCKET_H

#include <linuxmt/uio.h>

struct sockaddr
{
	unsigned short		sa_family;
	char			sa_data[14];
};

struct msghdr
{
	void	*	msg_name;
	int		msg_namelen;
	struct iovec *	msg_iov;
	int		msg_iovlen;
	void	*	msg_control;
	int		msg_controllen;
	int		msg_flags;
};

#define AF_INET	0	/* Only implemented type */

#define PF_INET	AF_INET

#define SOCK_STREAM     1               /* stream (connection) socket   */
#define SOCK_DGRAM      2               /* datagram (conn.less) socket  */
#define SOCK_RAW        3               /* raw socket                   */
#define SOCK_RDM        4               /* reliably-delivered message   */
#define SOCK_SEQPACKET  5               /* sequential packet socket     */


#endif /* LINUXMT_SOCKET_H */
