#include <ctype.h>

int isspace(int c)
{
	return (unsigned) (c - 9) < 5u || c == ' ';
}
