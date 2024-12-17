#include <malloc.h>
#include <string.h>

/* this realloc usable with all malloc allocators */
void *realloc(void *ptr, size_t size)
{
	void *nptr;
    size_t osize;

	if (ptr == 0)
		return malloc(size);

	osize = malloc_usable_size(ptr);
#if LATER
	if (size <= osize)
		return ptr;
#endif

	nptr = malloc(size);
	if (nptr == 0)
		return 0;

	memcpy(nptr, ptr, osize);
	free(ptr);

	return nptr;
}
