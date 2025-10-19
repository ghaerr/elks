#if VERBOSE
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
__noise(char *str, mem __wcnear *ptr)
{
    int saved_errno = errno;
    write(2, "Malloc ", 7);
    phex(ptr? (unsigned int)m_size(ptr): 0);
    write(2, " ptr ", 5);
    phex((int)ptr);
    write(2, " nxt ", 5);
    phex(ptr? (unsigned int)m_next(ptr): 0);
    write(2, " ", 1);
    write(2, str, strlen(str));
    write(2, "\n", 1);
    errno = saved_errno;
}
#endif
