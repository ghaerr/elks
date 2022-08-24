#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/*
 * connect with timeout
 *
 * Feb 2022 Greg Haerr
 */

static void alarm_cb(int sig)
{
	/* no action */
}

int in_connect(int socket, const struct sockaddr *address, socklen_t address_len,
		int secs)
{
	int ret;
	sighandler_t old;

	old = signal(SIGALRM, alarm_cb);
	alarm(secs);
	ret = connect(socket, address, address_len);
	alarm(0);
	signal(SIGALRM, old);
	return ret;
}
