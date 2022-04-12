#include <ctype.h>

int isprint(int c)
{
	return (c & 0x7f) >= 32 && c < 127;
}
