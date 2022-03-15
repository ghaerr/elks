#include <stdlib.h>

char *ltoa(long i)
{
	int   sign = (i < 0);
	static char a[34];
	char *b = a + sizeof(a) - 1;

	if (sign)
		i = -i;
	*b = 0;
	do {
		*--b = '0' + (i % 10);
		i /= 10;
	}
	while (i);
	if (sign)
		*--b = '-';
	return b;
}
