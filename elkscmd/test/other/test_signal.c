#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static int _signo;

void sigint (int signo)
{
	_signo = signo;
}

extern sighandler_t _sigtable [15];

int main (int argc, char ** argv)
{
	puts ("before signal()");
	printf ("SIGINT=%p\n", _sigtable [1]);

	signal (SIGINT, sigint);
	puts ("after signal()");
	printf ("SIGINT=%p\n", _sigtable [1]);
	printf ("sigint=%p\n", sigint);

	while (!_signo) sleep (1);
	printf ("exiting:signo=%u\n", _signo);
	return 0;
}
