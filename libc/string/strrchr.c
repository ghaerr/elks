#include <string.h>

char *
strrchr (const char * s, int c)
{
    char *save;
    char ch;

    for (save = (char *) 0; (ch = *s); s++) {
        if (ch == c)
            save = (char *) s;
    }
    return save;
}

#ifdef __GNUC__
char *rindex(const char *s, int c) __attribute__ ((weak, alias ("strrchr")));
#endif
