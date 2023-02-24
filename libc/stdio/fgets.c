#include <stdio.h>

char *
fgets(char *s, size_t count, FILE *f)
{
    char *ret;
    register size_t i;
    register int ch;

    ret = s;
    for (i = count-1; i > 0; i--) {
        ch = getc(f);
        if (ch == EOF) {
            if (s == ret)
                return 0;
            break;
        }
        *s++ = (char) ch;
        if (ch == '\n')
            break;
    }
    *s = 0;

    if (ferror(f))
        return 0;
    return ret;
}
