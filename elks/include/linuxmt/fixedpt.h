#ifndef __LINUXMT_FIXEDPT_H
#define __LINUXMT_FIXEDPT_H

/*
 * Fixed point for CPU usage calculations
 * Uses fixed point 10.11 arithmetic in 32 bit unsigned long:
 *  10 bit integer, 11 bit fraction, up to 1024 with 1/2048 (=.00048828) resolution.
 *  11 bit fractions expand to 22 bits when multiplied, plus 10 bits = 32 bits.
 *  Integer overflow will occur over 10 bits (1024).
 * CPU usage is calculated every 2 seconds, thus max integer value is 200 (=8 bits).
 *
 * exponential decay math for cpu %
 * desc         sample       total    fraction     decimal  decimal*2^11
 * 2sec/2sec    2 secs       2 secs   1/e^(2/2)    0.3679   753
 *
 */
#define SAMP_FREQ   (2 * HZ)        /* 2 sec intervals */
#define FSHIFT      11              /* nr of bits of precision */
#define FIXED_1     (1<<FSHIFT)     /* 1.0 as fixed-point */
#define FIXED_HALF  (1<<(FSHIFT-1)) /* 0.5 as fixed point */

/* exponential factor is amount kept between cycles, add new N at (1 - factor) */
//#define EXP_E     (0.3679 * (1 << FSHIFT))    /* 1e^(2sec/2sec = 36.79% factor */
#define EXP_E       753         /* 36.79% factor 1/e^(2sec/2sec) as fixed-point */
#define EXP_125     256         /* 12.50% factor (256/2048) */
#define EXP_008     16          /*  0.80% factor (16/2048) */

/* Add N to fixed point average using exp factor. N is in FIXED_1 units */
#define CALC_USAGE(fixednum,exp,N) \
    fixednum *= exp;                 /* multiply by exponential factor for fp10.22 */ \
    fixednum += (N)*(FIXED_1-(exp)); /* add in N using (1-exp) factor, still fp10.22 */ \
    fixednum >>= FSHIFT;             /* result is fp10.11 */

/* integer and percentage fractional values of fixed point number */
#define FIXED_INT(x)     ((unsigned int)((x) >> FSHIFT))
#define FIXED_FRAC(x)    FIXED_INT(((x) & (FIXED_1-1)) * 100)

#endif
