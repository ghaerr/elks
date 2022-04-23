#include <ctype.h>

int islower(int c)
{
	return (unsigned) (c - 'a') < 26u;
}
