/* stub to force inclusion of debug malloc (v7) */
#include <stdlib.h>

void *malloc(size_t size)
{
    return _dmalloc(size);
}

void free(void *ptr)
{
    _dfree(ptr);
}

void *realloc(void *ptr, size_t size)
{
    return _drealloc(ptr, size);
}
