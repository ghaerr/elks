#include <unistd.h>
#include <string.h>

void strip_trailing_slashes(char *path)
{
	int last;

	last = strlen (path) - 1;
	while (last > 0 && path[last] == '/')
		path[last--] = '\0';
}

