/* time command - modified from BSD 4.2 by Greg Haerr*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>

/* return current time in microseconds */
clock_t time_usecs(void)
{
        struct timeval tv;

        if (gettimeofday(&tv, (void *)0) < 0)
                return 0;

        return (tv.tv_sec * 1000000LL + tv.tv_usec);
}

static void print_time(char * s, clock_t us)
{
	long mins, secs;

	if (us < 1000 && us > 499)	/* round up to 1/1000 second*/
		us = 1000;
	mins = us / 60000000LL;
	if (mins)
		us -= mins * 60000000LL;

	secs = us / 1000000L;
	if (secs)
		us -= secs * 1000000LL;

	fprintf(stderr, "%s\t%lum%lu.%03lus\n", s, mins, secs, (long)us/1000);
	
}


int main(int argc, char **argv)
{
	int status, p;
	clock_t start, end;

	if(argc <= 1) {
		fprintf(stderr, "Usage: time <program ...>\n");
		return 0;
	}

	start = time_usecs();
	p = fork();
	if(p == -1) {
		perror(NULL);
		return 1;
	}
	if(p == 0) {
		execvp(argv[1], &argv[1]);
		perror(argv[1]);
		return 1;
	}
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	while (waitpid(p, &status, 0) != p)
		continue;
	if((status&0377) != 0)
		fprintf(stderr,"Command terminated abnormally.\n");
	end = time_usecs();
	fprintf(stderr, "\n");
	print_time("Real", end - start);
	exit(status>>8);
}
