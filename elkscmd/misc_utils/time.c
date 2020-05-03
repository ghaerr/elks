/* time command - modified from BSD 4.2 by Greg Haerr*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/wait.h>

static void printt(char * s, long us)
{
	long mins, secs;

	if (us < 1000L && us > 499L)	/* round up to 1/1000 second*/
		us = 1000L;
	mins = us / 60000000L;
	if (mins)
		us -= mins * 60000000L;

	secs = us / 1000000L;
	if (secs)
		us -= secs * 1000000L;

	fprintf(stderr, "%s\t%lum%lu.%03lus\n", s, mins, secs, us/1000);
	
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
	while (waitpid(p, &status, 0) != p)
		continue;
	if((status&0377) != 0)
		fprintf(stderr,"Command terminated abnormally.\n");
	times(&end);
	fprintf(stderr, "\n");
	printt("Real", end.tms_cstime - start.tms_cstime);
	exit(status>>8);
}
