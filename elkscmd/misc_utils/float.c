#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

__STDIO_PRINT_FLOATS;	/* force float libc printf/sprintf support */

#define FLOAT	float
#define VOLATILE volatile
VOLATILE FLOAT f = 1;
VOLATILE FLOAT g = 2.0;

extern void z(float);
//extern xprintf(char *, ...);
extern xprintf(char *, float, float);
void x(float f)
{
	//z(f);
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

int main(int argc, char **argv) {
	FLOAT f = 3.00 * g;
	int decpt, neg;
	printf("%s\n", fcvt(f, 12, &decpt, &neg));

	char buf[32];
	__fp_print_func(f, 'g', -1, buf);
	printf("%s, %g, %f, %e\n", buf, f, f, f);

	printf("float = %d\n", sizeof(float));
	printf("LONG_MAX = %ld,%ld\n", LONG_MAX, LONG_MIN);
	printf("3.1415926 = %f\n", strtod("3.1415926", 0));
	printf("3.1415926 = %f\n", (FLOAT)3.1415926);
	printf("2e5 = %g\n", strtod("2e5", 0));
	printf("floor(3.1415926) = %f\n", host_floor(3.1415926));
	printf("floor(-3.1415926) = %f\n", host_floor(-3.1415926));

	//f = (float)3.1415926;
	f = 3.1515926;
	g = f * g;
	printf("2 x %g = %g\n", (float)f, (float)g);
	x(f);

	return 0;
}
