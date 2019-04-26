#include <stdio.h>
#include <stdlib.h>

void
srand(unsigned int seed)
{
}

int
rand(void)
{
	static int x = 1;
	return ++x; //time();
}
