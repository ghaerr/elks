#include <stdio.h>

FILE * fopen (const char * file, const char * mode)
{
	return __fopen(file, -1, (FILE*)0, mode);
}
