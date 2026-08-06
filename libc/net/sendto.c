/*
 * sendto/send over the 5-argument _sendto syscall.
 *
 * An ELKS syscall carries at most five arguments (bx,cx,dx,di,si), and POSIX
 * sendto takes six. The kernel entry therefore drops addrlen and assumes
 * sizeof(struct sockaddr_in) - AF_INET is the only family implementing it.
 * This restores the POSIX shape, exactly as getsocknam.c does for
 * getsockname/getpeername.
 */
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
