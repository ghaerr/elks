#include <malloc.h>
#include <string.h>

void *
calloc(unsigned int elm, unsigned int sz)
{
	register unsigned int v;
	register void *ptr;

	ptr = malloc(v = elm * sz);
	if(ptr)
		memset(ptr, 0, v);

	return ptr;
}
