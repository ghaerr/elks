#include <ctype.h>

int isalnum(int c)
{
	return (unsigned) ((c | 0x20) - 'a') < 26u ||
		(unsigned) (c - '0') < 10u;
}
