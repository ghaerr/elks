/*
 * (C) 1999 Al Riddoch <ajr@ecs.soton.ac.uk> under terms of LGPL
 */

#include <unistd.h>
#include <sys/time.h>

unsigned int sleep(seconds)
unsigned int seconds;
{
	struct timeval tval;
	tval.tv_sec = seconds;
	tval.tv_usec = 0;
	select(0, NULL, NULL, NULL, &tval);
}
