/* an elks syscall carries five args and posix sendto takes six, so the
 * kernel drops addrlen and assumes sizeof(struct sockaddr_in). this puts the
 * posix shape back, same as getsocknam.c does */
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

int _sendto(int, const void *, size_t, int, const struct sockaddr *);

int sendto(int fd, const void *buf, size_t len, int flags,
	const struct sockaddr *address, socklen_t address_len)
{
	if (address && address_len < sizeof(struct sockaddr_in)) {
		errno = EINVAL;
		return -1;
	}
	return _sendto(fd, buf, len, flags, address);
}

int send(int fd, const void *buf, size_t len, int flags)
{
	return _sendto(fd, buf, len, flags, (struct sockaddr *)0);
}
