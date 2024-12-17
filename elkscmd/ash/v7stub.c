/* stub to force inclusion of debug malloc (v7) */
#include <stdlib.h>

void *malloc(size_t size)
{
    return __dmalloc(size);
}

void free(void *ptr)
{
    __dfree(ptr);
}

void *realloc(void *ptr, size_t size)
{
    return __drealloc(ptr, size);
}
