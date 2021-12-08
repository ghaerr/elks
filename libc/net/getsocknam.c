#include <sys/socket.h>

/* actual system call */
extern int getsocknam(int socket, struct sockaddr * restrict address,
	socklen_t * restrict address_len, int peer);

int getsockname(int socket, struct sockaddr * restrict address,
	socklen_t * restrict address_len)
{
	return getsocknam(socket, address, address_len, 0);
}

int getpeername(int socket, struct sockaddr * restrict address,
	socklen_t * restrict address_len)
{
	return getsocknam(socket, address, address_len, 1);
}
