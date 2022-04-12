#include <ctype.h>

int isxdigit(int c)
{
	return (isdigit(c) || (toupper(c) >= 'A' && toupper(c) <= 'F'));
}
