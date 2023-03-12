#include <stdlib.h>
#include <ctype.h>

int atoi(const char *s)
{
    int n = 0;
    int neg = 0;

    while (*s == ' ' || *s == '\t')
        s++;
    switch (*s) {
        case '-': neg = 1;
        case '+': s++;
    }
    while ((unsigned) (*s - '0') <= 9u)
        n = n * 10 + *s++ - '0';
    return neg ? 0u - n : n;
}
