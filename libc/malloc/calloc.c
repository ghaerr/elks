#include <errno.h>
#include <malloc.h>
#include <string.h>

void *
calloc(unsigned int elm, unsigned int sz)
{
    unsigned int v;
    void *ptr;

#ifdef __GNUC__
    if (__builtin_umul_overflow(elm, sz, &v)) {
        errno = ENOMEM;
        return 0;
    }
#else
    v = elm * sz;
    if (sz != 0 && v / sz != elm) {
        errno = ENOMEM;
        return 0;
    }
#endif

    ptr = malloc(v);
    if (ptr)
        memset(ptr, 0, v);

    return ptr;
}
