#include <malloc.h>
#include <string.h>

void *realloc(void *ptr, size_t size)
{
	void *nptr;
    size_t osize;

	if (ptr == 0)
		return malloc(size);

	osize = malloc_usable_size(ptr);
#if 0
	if (size <= osize)
		return ptr;         /* don't reallocate, do nothing */
#else
	if (size <= osize)
        osize = size;       /* copy less bytes in memcpy below */
#endif

	nptr = malloc(size);
	if (nptr == 0)
		return 0;
	memcpy(nptr, ptr, osize);
	free(ptr);

	return nptr;
}
