#include <ctype.h>

int tolower(int c)
{
	return (unsigned) (c - 'A') < 26u ? c + ('a' - 'A') : c;
}
