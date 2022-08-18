#ifndef __RUNES__H
#define __RUNES__H

/* UCS-2/UCS-4 to UTF-8 conversion library */

#include <stdint.h>

#define RUNE_UTF32   0  /* A Rune is 16-bits for ELKS, 32-bits for Linux/UNIX */

#if RUNE_UTF32
typedef int32_t Rune;   /* full UTF-32 encoding */
#else
typedef uint16_t Rune;  /* UCS-2 encodings only */
#endif

int isvalidrune(Rune r);            /* is Rune valid? */
int runelen(Rune r);                /* UTF-8 length of Rune */
int runetostr(char *s, Rune r);     /* convert Rune to UTF-8 + NUL, return length */

#endif
