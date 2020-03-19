/* time command - modified from BSD 4.2 by Greg Haerr*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/wait.h>

static void printt(char *s, long a)
{
	long mins, secs;

	if (a < 1000)			/* round up to 1/1000 second*/
		a = 1000;

	mins = a / 60000000L;
	if (mins)
		a %= mins * 60000000L;

	secs = a / 1000000L;
	if (secs)
		a %= secs * 1000000L;

	fprintf(stderr, "%s\t%lum%lu.%03lus\n", s, mins, secs, a / 1000L);
}

int main(int argc, char **argv)
{
	int status, p;
	struct tms end, start;

	if(argc <= 1) {
		fprintf(stderr, "Usage: time <program ...>\n");
		exit(0);
	}

	times(&start);
	p = fork();
	if(p == -1) {
		fprintf(stderr, "Try again.\n");
		exit(1);
	}
	if(p == 0) {
		execvp(argv[1], &argv[1]);
		perror(argv[1]);
		exit(1);
	}
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	while(wait(&status) != p)
		times(&start);
	if((status&0377) != 0)
		fprintf(stderr,"Command terminated abnormally.\n");
	times(&end);

	fprintf(stderr, "\n");
	printt("real", end.tms_cutime - start.tms_cutime);
	exit(status>>8);
}
