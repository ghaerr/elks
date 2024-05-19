#ifndef __MATH_H
#define __MATH_H
/*
 * ELKS libc math header file
 *
 * Apr 2022 Greg Haerr
 */

#define M_E         2.7182818284590452354   /* e */
#define M_LOG2E     1.4426950408889634074   /* log 2e */
#define M_LOG10E    0.43429448190325182765  /* log 10e */
#define M_LN2       0.69314718055994530942  /* log e2 */
#define M_LN10      2.30258509299404568402  /* log e10 */
#define M_PI        3.14159265358979323846  /* pi */
#define M_PI_2      1.57079632679489661923  /* pi/2 */
#define M_PI_4      0.78539816339744830962  /* pi/4 */
#define M_1_PI      0.31830988618379067154  /* 1/pi */
#define M_2_PI      0.63661977236758134308  /* 2/pi */
#define M_2_SQRTPI  1.12837916709551257390  /* 2/sqrt(pi) */
#define M_SQRT2     1.41421356237309504880  /* sqrt(2) */
#define M_SQRT1_2   0.70710678118654752440  /* 1/sqrt(2) */

double floor(double x);		/* round to largest integral value not greater than x */
float floorf(float x);

double sqrt(double x);		/* square root function */
float sqrtf(float x);

double fabs(double x);		/* absolute value function */
float fabsf(float x);

double exp(double x);		/* e**x, base-e exponential */
float expf(float x);

double exp2(double x);		/* 2**x, base-2 exponential */
float exp2f(float x);

double pow(double x, double y);	/* x**y, x raised to the power of y */
float powf(float x, float y);

double log(double x);		/* natural logarithm */
float logf(float x);

double log2(double x);		/* base 2 logarithm */
float log2f(float x);

double log10(double x);		/* base 10 logarithm */
float log10f(float x);

double cos(double x);		/* cosine function */
float cosf(float x);

double sin(double x);		/* sine function */
float sinf(float x);

double tan(double x);		/* tangent function */
float tanf(float x);

double acos(double x);		/* arc cosine function */
float acosf(float x);

double asin(double x);		/* arc sine function */
float asinf(float x);

double atan(double x);		/* arc tangent of one variable */
float atanf(float x);

float fminf(float x, float y);
float fmaxf(float x, float y);
#endif
