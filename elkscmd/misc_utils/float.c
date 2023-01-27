#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include <sys/linksym.h>
__STDIO_PRINT_FLOATS;       /* link in libc printf/sprintf float support */

#define FLOAT	float
#define VOLATILE volatile
VOLATILE FLOAT f = 1;
VOLATILE FLOAT g = 2.0;

extern void z(float);
//extern xprintf(char *, ...);
extern xprintf(char *, float, float);
void x(float f)
{
}

char *host_floatToStr(float f, char *buf) {
#if 1
    sprintf(buf, "%g", (double)f);
    return buf;
#else
    // floats have approx 7 sig figs
    float a = fabs(f);
    if (f == 0.0f) {
        buf[0] = '0';
        buf[1] = 0;
    }
    else if (a<0.0001 || a>1000000) {
        // this will output -1.123456E99 = 13 characters max including trailing nul
        dtostre(f, buf, 6, 0);
    }
    else {
        int decPos = 7 - (int)(floor(log10(a))+1.0f);
        dtostrf(f, 1, decPos, buf);
        if (decPos) {
            // remove trailing 0s
            char *p = buf;
            while (*p) p++;
            p--;
            while (*p == '0') {
                *p-- = 0;
            }
            if (*p == '.') *p = 0;
        }
    }
    return buf;
#endif
}

/* LONG_MAX is not exact as a double, yet LONG_MAX + 1 is */
#define LONG_MAX_P1 ((LONG_MAX/2 + 1) * 2.0)

/* small ELKS floor function using 32-bit longs */
float host_floor(float x)
{
  if (x >= 0.0) {
    if (x < LONG_MAX_P1) {
      return (float)(long)x;
    }
    return x;
  } else if (x < 0.0) {
    if (x >= LONG_MIN) {
      long ix = (long) x;
      return (ix == x) ? x : (float)(ix-1);
    }
    return x;
  }
  return x; // NAN
}

/* fixed point math experimentation for calculating load averages */
/*
 * These are the constant used to fake the fixed-point load-average
 * counting. Some notes:
 *  - 11 bit fractions expand to 22 bits by the multiplies: this gives
 *    a load-average precision of 10 bits integer + 11 bits fractional
 *  - if you want to count load-averages more often, you need more
 *    precision, or rounding will get you. With 2-second counting freq,
 *    the EXP_n values would be 1981, 2034 and 2043 if still using only
 *    11 bit fractions.
 */
#define HZ          100         /* jiff_t tick = 1/100 second */
#define SAMP_FREQ   (5*HZ)      /* 5 sec intervals */

typedef unsigned long fixed_point;  /* 10 bit int, 11 bit fraction in 32 bits */
#define FSHIFT      11          /* nr of bits of precision */
#define FIXED_1     (1<<FSHIFT) /* 1.0 as fixed-point */
/*
 * exponential decay math for load avg, fractional bits = 11 (=1/2048 resolution)
 *      = e^-(sample_period/total_period) = 1/e^(sample_period/total_period)
 * desc         sample       total    fraction     decimal  decimal*2^11
 * 5sec/1min    5 secs       60 secs  1/e^(1/12)   0.9200   1884
 * 5sec/5min    5 secs      300 secs  1/e^(1/60)   0.9835   2014
 * 5sec/15min   5 secs      900 secs  1/e^(1/180)  0.9945   2037
 */
//#define EXP_1     (0.9200 * (1 << FSHIFT))
//#define EXP_5     (0.9835 * (1 << FSHIFT))
//#define EXP_15    (0.9945 * (1 << FSHIFT))
#define EXP_1       1884        /* 1/exp(5sec/1min) as fixed-point */
#define EXP_5       2014        /* 1/exp(5sec/5min) */
#define EXP_15      2037        /* 1/exp(5sec/15min) */

//#define EXP_0     (0.3679 * (1 << FSHIFT))
#define EXP_0       753         /* 1/exp(2sec/2sec) as fixed-point */
/*
 * exponential decay math for cpu %
 * desc         sample       total    fraction     decimal  decimal*2^11
 * 2sec/2sec    2 secs       2 secs   1/e^(1/1)    0.3679   753
 *
 * process start:
 *  current->average = 0;
 *  current->ticks = 0;
 * each timer tick:
 *  current->ticks++;
 * after total interval (2 secs):
 *  CALC_LOAD(current->average, EXP_0, current->ticks << FSHIFT);
 *  current->ticks = 0;
 * to display:
 *  CPU % = (1/(HZ * 2)) * 100 = 1/2 of average:
 *  average = (average * 1024) >> FSHIFT
 *  average = (average << 10) >> FSHIFT
 *  average = average >> (FSHIFT - 10)
 *  average = average >> 1.
 */

/* Fixed point math:
 *
 * Fixed point sum is first multiplied by fractional exponential,
 * (i.e. original value) creating 10 bits integer + 22 bits fraction.
 *
 * Fixed point multiply of 10 bits integer and 11 bits fraction will
 * result in 22 bits fraction and 10 bits integer (ignoring additional
 * 10 bits integer overflow), which still fits in 32 bits.
 *
 * N, which is an integer multiple of FIXED_1, is then added in using
 * (1-exponential, i.e. the new value). FIXED_1 * (FIXED_1-exp) will
 * result in N being added above 22 bits fractional value.
 *
 * New value is then shifted right FSHIFT (11) bits to discard 11
 * bits of fractional underflow, resulting in new 10 bit integer/11 bit
 * fractional value again in 32 bit unsigned long.
 */
#define CALC_LOAD(load,exp,N) \
    load *= exp;                /* multiply by 1/e^(sample/total) factor */ \
    load += N*(FIXED_1-exp);    /* N is in FIXED_1 units */ \
    load >>= FSHIFT;

/* example load average use in kernel timer routine */
#if 0
fixed_point average[3];     /* Load averages */
void timer_tick()
{
    static int count = SAMP_FREQ;

    if (count-- <= 0) {
        count = SAMP_FREQ;
        fixed_point N = FIXED_1 * count_active_tasks();

        CALC_LOAD(average[0], EXP_1, N);
        CALC_LOAD(average[1], EXP_5, N);
        CALC_LOAD(average[2], EXP_15, N);
    }
}

/* integer and percentage fractional values of fixed point number */
#define LOAD_INT(x)     ((x) >> FSHIFT)
#define LOAD_FRAC(x)    LOAD_INT(((x) & (FIXED_1-1)) * 100)

void print_loadavg()
{
    int a, b, c;
    a = average[0] + (FIXED_1/(HZ * 2));    /* round by adding 0.5 interval */
    b = average[1] + (FIXED_1/(HZ * 2));
    c = average[2] + (FIXED_1/(HZ * 2));
    printf("%d.%02d %d.%02d %d.%02d\n",
        LOAD_INT(a), LOAD_FRAC(a),
        LOAD_INT(b), LOAD_FRAC(b),
        LOAD_INT(c), LOAD_FRAC(c));
}
#endif


int main(int argc, char **argv) {
	FLOAT f = 3.00 * g;
	int decpt, neg;
	printf("%s\n", fcvt(f, 12, &decpt, &neg));

	char buf[32];
	dtostr(f, 'g', -1, buf);
	printf("%s, %g, %f, %e\n", buf, f, f, f);

	printf("float = %d\n", sizeof(float));
	printf("LONG_MAX = %ld,%ld\n", LONG_MAX, LONG_MIN);
	printf("3.1415926 = %f\n", strtod("3.1415926", 0));
	printf("3.1415926 = %f\n", (FLOAT)3.1415926);
	printf("2e5 = %g\n", strtod("2e5", 0));

	volatile double pi = 3.1415926535;
	volatile double pio2 = 3.1415926535 / 2.0;
	volatile double one = 1.0;
	volatile double e = 2.718282;
	volatile double two = 2.0;
	volatile double three = 3.0;
	volatile double four = 4.0;

	printf("floor(3.1415926) = %f,%f\n", floor(pi), host_floor(pi));
	printf("floor(-3.1415926) = %f,%f\n", floor(-pi), host_floor(-pi));
	printf("exp(1) = %f\n", exp(one));
	printf("log(2.718282) = %f\n", log(e));
	printf("pow(3,4) = %f\n", pow(three, four));
	printf("sqrt(2) = %f\n", sqrt(two));

	volatile float pif = 3.1415926535;
	volatile float onef = 1.0;
	volatile float ef = 2.718282;
	volatile float twof = 2.0;
	volatile float threef = 3.0;
	volatile float fourf = 4.0;

	printf("\n");
	printf("floorf(3.1415926) = %f,%f\n", floorf(pif), host_floor(pif));
	printf("floorf(-3.1415926) = %f,%f\n", floorf(-pif), host_floor(-pif));
	printf("expf(1) = %f\n", expf(onef));
	printf("logf(2.718282) = %f\n", logf(ef));
	printf("powf(3,4) = %f\n", powf(threef, fourf));
	printf("sqrtf(2) = %f\n", sqrtf(twof));

	//tan(one);
	//asin(one);
	//acos(one);
	//atan(one);

	// beware: floating point literals are double
	// (float) cast doesn't stop promotion to double on varargs routines
	// gcc will use math builtin if argument is constant
	//f = (float)3.1415926;
	f = 3.1515926;
	g = f * g;
	printf("2 x %g = %g\n", (float)f, (float)g);
	x(f);

	volatile double d = 0;
	printf("cos(%f) = %f\n", d, cos(d));
	d += pio2;
	printf("cos(%f) = %f\n", d, cos(d));
	d += pio2;
	printf("cos(%f) = %f\n", d, cos(d));
	d += pio2;
	printf("cos(%f) = %f\n", d, cos(d));

    printf("%s\n", itoa(0x7fff));
    printf("%s\n", itoa(-0x7fff));
    printf("%s\n", itoa(0x8000));
    printf("%s\n", itoa(0xffff));

    printf("%s\n", ltoa(0x7fffffff));
    printf("%s\n", ltoa(-0x7fffffff));
    printf("%s\n", ltoa(0x80000000));
    printf("%s\n", ltoa(0xffffffff));

    printf("%s\n", ltostr(0x7fffffff, 10));
    printf("%s\n", ltostr(-0x7fffffff, 10));
    printf("%s\n", ltostr(0x80000000, 10));
    printf("%s\n", ltostr(0xffffffff, 10));

    printf("%s\n", ultostr(0x7fffffff, 10));
    printf("%s\n", ultostr(-0x7fffffff, 10));
    printf("%s\n", ultostr(0x80000000, 10));
    printf("%s\n", ultostr(0xffffffff, 10));
	return 0;
}
