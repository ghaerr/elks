#ifndef __SYS_SOCKET_H
#define __SYS_SOCKET_H

#include <features.h>
#include __SYSINC__(socket.h)

typedef unsigned int socklen_t;

int accept (int socket, struct sockaddr * restrict address, socklen_t * restrict address_len);
int bind (int socket, const struct sockaddr * address, socklen_t address_len);
int connect (int socket, const struct sockaddr * address, socklen_t address_len);
int listen (int socket, int backlog);
int socket (int domain, int type, int protocol);
int setsockopt(int socket, int level, int option_name, const void *option_value,
	socklen_t option_len);
int getsockname (int socket, struct sockaddr * restrict address,
	socklen_t * restrict address_len);
int getpeername (int socket, struct sockaddr * restrict address,
	socklen_t * restrict address_len);

#endif
