#include <stdio.h>

FILE * fopen (char * file, char * mode)
{
	return __fopen(file, -1, (FILE*)0, mode);
}
