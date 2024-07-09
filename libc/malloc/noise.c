#if defined(VERBOSE)
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "_malloc.h"

/* NB: Careful here, stdio may use malloc - so we can't */
static void
phex(unsigned int val)
{
    static char hex[] = "0123456789ABCDEF";
    int i;

    for (i = sizeof(unsigned int) * 8 - 4; i >= 0; i -= 4)
        write(2, hex + ((val >> i) & 0xF), 1);
}

void
__noise(char *y, mem __wcnear *x)
{
    int saved_errno = errno;
    write(2, "Malloc ", 7);
    phex((int)x);
    write(2, " sz ", 4);
    if (x)
        phex((unsigned int)m_size(x));
    else
        phex(0);
    write(2, " nxt ", 5);
    if (x)
        phex((unsigned int)m_next(x));
    else
        phex(0);
    write(2, " is ", 4);
    write(2, y, strlen(y));
    write(2, "\n", 1);
    errno = saved_errno;
}
#endif
