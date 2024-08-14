/*
 * Precision timing pticks (=0.838us) output from 'k' format.
 */

static char hex_string[] = "0123456789ABCDEF 0123456789abcdef ";

void ptostr(unsigned long v, char *buf)
{
    unsigned long dvr;
    int c, vch, i;
    int Msecs, Decimal, Zero;
    int width = 0;

    i = 10;
    dvr = 1000000000L;
    Zero = 0;
    Msecs = 0;
    /* display 1/1193182s get_time*() pticks in range 0.838usec through 42.85sec */
    Decimal = 3;
    if (v > 51130563UL)             /* = 2^32 / 84 high max range = ~42.85s */
        v = 0;                      /* ... displays 0us */
    if (v > 5125259UL) {            /* = 2^32 / 838 */
        v = v * 84UL;
        Decimal = 2;                /* display xx.xx secs */
    } else
        v = v * 838UL;              /* convert to nanosecs w/o 32-bit overflow */
    if (v > 1000000000UL) {         /* display x.xxx secs */
        v /= 1000000UL;             /* divide using _udivsi3 for speed */
    } else if (v > 1000000UL) {
        v /= 1000UL;                /* display xx.xxx msecs */
        Msecs = 1;
    } else {
        Msecs = 2;                  /* display xx.xxx usecs */
    }

    vch = 0;
    do {
        c = (int)(v / dvr);
        v %= dvr;
        dvr /= 10U;
        if (c || (i <= width) || (i < 2)) {
            if (i > width)
                width = i;
            if (!Zero && !c && (i > 1))
                c = 16;
            else
                Zero = 1;
            if (vch)
                *buf++ = vch;
            vch = *(hex_string + c);
            if (i == Decimal) *buf++ = '.';
        }
    } while (--i);
    *buf++ = vch;
    if (Msecs)
        *buf++ = (Msecs == 1? 'm': 'u');
    *buf++ = 's';
        *buf = '\0';
}
