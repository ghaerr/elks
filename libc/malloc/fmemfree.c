#include <malloc.h>

/* free from main memory */
int fmemfree(void __far *ptr)
{
    return _fmemfree((unsigned short)((unsigned long)ptr >> 16));
}
