#include <ctype.h>
#include <stdio.h>

int isprint(int c)
{
    if (c == EOF) return 0;
    return (c & 0xff) >= 32;    /* display ASCII and upper half of CP 437 */
}
