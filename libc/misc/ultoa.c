#include <stdlib.h>
#define MAX_LONG_CHARS 34

/* small unsigned long to ascii */
char *ultoa(unsigned long i)
{
    static char buf[MAX_LONG_CHARS];
    char *b = buf + sizeof(buf) - 1;

    *b = '\0';
    do {
        *--b = '0' + (i % 10);
    } while ((i /= 10) != 0);
    return b;
}
