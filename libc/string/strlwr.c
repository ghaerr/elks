#include <string.h>
#include <ctype.h>

char *
strlwr(char *str)
{
    unsigned char *p;

    for (p = (unsigned char *)str; *p != '\0'; p++)
        *p = tolower(*p);
    return str;
}
