#include <ctype.h>

int isalpha(int c)
{
	return (unsigned) ((c | 0x20) - 'a') < 26u;
}
