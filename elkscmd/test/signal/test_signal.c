
#include <unistd.h>
#include <signal.h>

static int _signo = 0;

void sigint (int signo)
{
	_signo = signo;
}

extern __sighandler_t __sigtable [15];

int main (int argc, char ** argv)
{
	sig_t sigold;

	puts ("before signal()");
	printf ("SIGINT=%u\n", __sigtable [1]);

	sigold = signal (SIGINT, sigint);
	puts ("after signal()");
	printf ("SIGINT=%u\n", __sigtable [1]);
	printf ("sigint=%u\n", sigint);

	while (!_signo) sleep (1);
	printf ("exiting:signo=%u\n", _signo);
	return 0;
}
