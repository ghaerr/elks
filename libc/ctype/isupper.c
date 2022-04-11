#include <ctype.h>

int isupper(int c)
{
	return (unsigned) (c - 'A') < 26u;
}
