#include <ctype.h>

int toupper(int c)
{
	return (unsigned) (c - 'a') < 26u ? c + ('A' - 'a') : c;
}
