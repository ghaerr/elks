#ifndef __C86_LIMITS_H
#define __C86_LIMITS_H

/* limits.h ANSI required limits for C86 */
#define CHAR_BIT    8                /* number of bits in a byte           */
#define CHAR_MIN   (-128)            /* minimum value of a char            */
#define CHAR_MAX   127               /* maximum value of a char            */
#define SCHAR_MIN   (-128)           /* minimum value of a signed char     */
#define SCHAR_MAX   127              /* maximum value of a signed char     */
#define UCHAR_MAX   255              /* maximum value of an unsigned char  */

#define SHRT_MIN   (-32767-1)        /* minimum value of a short           */
#define SHRT_MAX   32767             /* maximum value of a short           */
#define USHRT_MAX  65535             /* maximum value of an unsigned short */
#define INT_MIN    (-2147483647-1)   /* minimum value of an int            */
#define INT_MAX    2147483647        /* maximum value of an int            */
#define UINT_MAX   4294967295U       /* maximum value of an unsigned int   */
#define LONG_MIN   (-2147483647L-1L) /* minimum value of a long            */
#define LONG_MAX   2147483647L       /* maximum value of a long            */
#define ULONG_MAX  4294967295UL      /* maximum value of an unsigned long  */

#endif
