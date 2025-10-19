#include <sys/socket.h>

/* actual system call */
extern int getsocknam(int sock, struct sockaddr * restrict address,
	socklen_t * restrict address_len, int peer);

int getsockname(int sock, struct sockaddr * restrict address,
	socklen_t * restrict address_len)
{
	return getsocknam(sock, address, address_len, 0);
}

int getpeername(int sock, struct sockaddr * restrict address,
	socklen_t * restrict address_len)
{
	return getsocknam(sock, address, address_len, 1);
}
