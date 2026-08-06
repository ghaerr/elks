#ifndef __SYS_SOCKET_H
#define __SYS_SOCKET_H

#include <features.h>
#include __SYSINC__(socket.h)

typedef unsigned int socklen_t;

int accept (int sock, struct sockaddr * restrict address, socklen_t * restrict address_len);
int bind (int sock, const struct sockaddr * address, socklen_t address_len);
int connect (int sock, const struct sockaddr * address, socklen_t address_len);
int listen (int sock, int backlog);
int socket (int domain, int type, int protocol);
int setsockopt (int sock, int level, int option_name, const void *option_value,
	socklen_t option_len);
int getsocknam (int sock, struct sockaddr * restrict address, /* syscall for two below */
    socklen_t * restrict address_len, int peer);
int getsockname (int sock, struct sockaddr * restrict address,
	socklen_t * restrict address_len);
int getpeername (int sock, struct sockaddr * restrict address,
	socklen_t * restrict address_len);

/* datagram sockets. The kernel entries take five arguments (ELKS syscalls
 * carry no more); these restore the POSIX six-argument shape. */
int sendto (int sock, const void *buf, size_t len, int flags,
	const struct sockaddr *address, socklen_t address_len);
int recvfrom (int sock, void *buf, size_t len, int flags,
	struct sockaddr * restrict address, socklen_t * restrict address_len);
int send (int sock, const void *buf, size_t len, int flags);
int recv (int sock, void *buf, size_t len, int flags);

#endif
