#include <stdlib.h>

/* small unsigned int to ascii */
char *uitoa(unsigned int i)
{
    static char buf[6];
    char *b = buf + sizeof(buf) - 1;

    *b = '\0';
    do {
        *--b = '0' + (i % 10);
    } while ((i /= 10) != 0);
    return b;
}
