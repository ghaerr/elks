#include <stdlib.h>
#define MAX_LONG_CHARS 34

/* small unsigned long to ascii */
char *ultoa(unsigned long i)
{
    static char buf[MAX_LONG_CHARS];
    char *b = buf + sizeof(buf) - 1;

    *b = '\0';
#ifdef _M_I86
    do {
        unsigned int c;
        c = 10;
        i = __divmod(i, &c);
        *--b = '0' + c;
    } while (i);
#else
    do {
        *--b = '0' + (i % 10);
    } while ((i /= 10) != 0);
#endif
    return b;
}
