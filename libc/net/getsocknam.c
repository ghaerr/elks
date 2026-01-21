#include <sys/socket.h>

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
