/*
 * recvfrom/recv over the 5-argument _recvfrom syscall. See sendto.c for why
 * addrlen is not passed to the kernel.
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

int _recvfrom(int, void *, size_t, int, struct sockaddr *);

int recvfrom(int fd, void *buf, size_t len, int flags,
	struct sockaddr *address, socklen_t *address_len)
{
	int ret;

	if (address && (!address_len || *address_len < sizeof(struct sockaddr_in))) {
		errno = EINVAL;
		return -1;
	}
	ret = _recvfrom(fd, buf, len, flags, address);
	if (ret >= 0 && address && address_len)
		*address_len = sizeof(struct sockaddr_in);
	return ret;
}

int recv(int fd, void *buf, size_t len, int flags)
{
	return _recvfrom(fd, buf, len, flags, (struct sockaddr *)0);
}
