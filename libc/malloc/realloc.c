#include <malloc.h>
#include <string.h>

#include "_malloc.h"

#undef malloc

void *
realloc(void *ptr, size_t size)
{
	void *nptr;
	unsigned int osize;

	if (ptr == 0)
		return malloc(size);

	osize = (m_size(((mem *) ptr) - 1) - 1) * sizeof(mem);

	if (size <= osize)
		return ptr;

	nptr = malloc(size);

	if (nptr == 0)
		return 0;

	memcpy(nptr, ptr, osize);
	free(ptr);

	return nptr;
}
