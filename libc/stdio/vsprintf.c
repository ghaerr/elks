#include <stdio.h>

int
vsprintf(char *sp, const char *fmt, va_list ap)
{
	static FILE string[1] =
	{
		{
			0,
			0,
			(unsigned char *)(unsigned)-1,
			0,
			(unsigned char *)(unsigned)-1,
			-1,
		 _IOFBF | __MODE_WRITE
		 }
	};

	int rv;

	string->bufpos = (unsigned char *)sp;
	rv = vfprintf(string, fmt, ap);
	*(string->bufpos) = 0;

	return rv;
}
