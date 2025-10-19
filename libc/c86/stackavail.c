#include <alloca.h>
/* C86 stack checking helper routines */

/* Return true if stack can be extended by size bytes */
int __stackavail(unsigned int size)
{
    unsigned int remaining = (unsigned)&size - __stacklow;

    if ((int)remaining >= 0 && remaining >= size)
        return 1;
    return 0;
}
