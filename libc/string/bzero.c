#include <string.h>

void bzero (void * s, size_t n)
{
	(void) memset (s, '\0', n);
}
