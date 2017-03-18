/*
 * Connect to the server, then wait until terminated (useful for stopping the server
 * from exiting without a window manager)
 */

#include "nano-X.h"

int main()
{
	char c;
	read(GrOpen(), &c, 1);
	return 0;
}
