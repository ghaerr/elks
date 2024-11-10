/*
 * Convert precision timing pticks (=0.838us) to string for printf %k format.
 * This routine is normally weak linked into vfprintf by a get_ptime reference.
 */
#include <stdlib.h>

void ptostr(unsigned long v, int alt, char *outbuf)
{
    unsigned int c;
    char *p;
    int n, Decimal, Suffix = 0;
    char buf[12];

    /* display 1/1193182s get_ptick() pticks in range 0.838usec through 42.85sec */
    if (v > 5125259UL) {            /* = 2^32 / 838  = ~4.3s */
        if (v > 51130563UL)         /* = 2^32 / 84 high max range = ~42.85s */
            v = 0;                  /* ... displays 0us */
        v *= 84;
        Decimal = 2;                /* display xx.xx secs */
    } else {
        v *= 838;                   /* convert to nanosecs w/o 32-bit overflow */
        Decimal = 3;                /* display xx.xxx */
    }
    while (v > 1000000L) {          /* quick divides for readability */
        c = 1000;
        v = __divmod(v, &c);
        Suffix++;
    }
    if (Suffix == 0)      Suffix = ('s' << 8) | 'u';
    else if (Suffix == 1) Suffix = ('s' << 8) | 'm';
    else                  Suffix = 's';

    p = buf + sizeof(buf) - 1;
    *p = '\0';
    do {
        c = 10;
        v = __divmod(v, &c);        /* remainder returned in c */
        *--p = '0' + c;
    } while (v != 0);

    n = buf + sizeof(buf) - 1 - p;  /* string length */
    while (*p) {
        if (n == Decimal) {
            if (alt)
                break;
            *outbuf++ = '.';
        }
        --n;
        *outbuf++ = *p++;
    }
    while (Suffix) {
        *outbuf++ = Suffix & 255;
        Suffix >>= 8;
    }
    *outbuf = '\0';
}
