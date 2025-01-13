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
int setsockopt(int sock, int level, int option_name, const void *option_value,
	socklen_t option_len);
int getsockname (int sock, struct sockaddr * restrict address,
	socklen_t * restrict address_len);
int getpeername (int sock, struct sockaddr * restrict address,
	socklen_t * restrict address_len);

#endif
