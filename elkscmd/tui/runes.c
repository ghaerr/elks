/*
 * UCS-2/UCS-4 to UTF-8 conversion library
 *
 * from libutf (https://github.com/cls/libutf)
 *
 * MIT License
 * 
 * © 2012-2016 Connor Lane Smith <cls@lubutu.com>
 * © 2015 Laslo Hunhold <dev@frign.de>
 * © 2022 adapted by Greg Haerr
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */
#include "runes.h"

int isvalidrune(Rune r)
{
#if RUNE_UTF32
    if(r < 0)
        return 0; /* negative value */

    if((r & 0xFFFE) == 0xFFFE)
        return 0; /* non-character at end of plane */

    if(r > 0x10FFFF)
        return 0; /* too large, thanks to UTF-16 */
#else
    if(r > 0xFFFD)
        return 0; /* too large, not UCS-2 */
#endif
    if(r >= 0xD800 && r <= 0xDFFF)
        return 0; /* surrogate pair range */

    if(r >= 0xFDD0 && r <= 0xFDEF)
        return 0; /* non-character range */

    return 1;
}

/* UTF-8 length of Rune */
int runelen(Rune r)
{
    if(!isvalidrune(r))
        return 0;
    else if(r < (1 << 7))
        return 1;
    else if(r < (1 << 11))
        return 2;
#if RUNE_UTF32
    else if(r < (1 << 16))
        return 3;
    else if(r < (1 << 21))
        return 4;
    else if(r < (1 << 26))
        return 5;
    else
        return 6;
#else
    return 3;   /* non-UCS-2 checked in isvalidrune() */
#endif
}

/* convert Rune to UTF-8 + NUL, return length */
int runetostr(char *s, Rune r)
{
    unsigned char i, n, x;

    n = runelen(r);

    if(n == 1) {
        s[0] = r;
        s[1] = '\0';
        return 1;
    }

    if(n == 0)
        return 0;

    for(i = n; --i > 0; r >>= 6)
        s[i] = 0200 | (r & 077);

    x = 0377 >> n;
    s[0] = ~x | r;
    s[n] = '\0';
    return n;
}
