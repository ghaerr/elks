#include <string.h>
#include <ctype.h>

char *
strupr(char *str)
{
    unsigned char *p;

    for (p = (unsigned char *)str; *p != '\0'; p++)
        *p = toupper(*p);
    return str;
}
