#include <stdio.h>
#include <stdlib.h>

__STDIO_PRINT_FLOATS;	/* force float libc printf/sprintf support */

double g = 2.0;

int main(int argc, char **argv) {
	double f = 3.00 * g;
	//int decpt, neg;
	//char *s = fcvt(f, 12, &decpt, &neg);
	char buf[32];
	__fp_print_func(f, 'g', -1, buf);
	printf("%s, %g, %f, %e\n", buf, f, f, f);

	return 0;
}
