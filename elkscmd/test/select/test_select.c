#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>


int main (int argc, char ** argv)
{
    fd_set fds;
    struct timeval tv;
    int count;

    tv.tv_sec  = 10;
    tv.tv_usec = 0;

	FD_ZERO (&fds);
	FD_SET (0, &fds);

	puts ("before select()");
	count = select (1, &fds, NULL, NULL, &tv);
	puts ("after select()");

	if (count < 0)
		puts ("select error");
	else
		puts ((count > 0) ? "count > 0" : "count = 0");

	if (FD_ISSET (0, &fds)) puts ("input ready");

	return 0;
}
