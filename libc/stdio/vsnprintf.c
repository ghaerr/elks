#include <stdio.h>

int
vsnprintf(char *sp, size_t size, const char *fmt, va_list ap)
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

	char bogo[1];

	if (size == 0)
	{
		sp = bogo;
		size = 1;
	}

	string->bufpos = (unsigned char *)sp;
	string->bufwrite = string->bufend = (unsigned char *)(sp + size - 1);
	rv = vfprintf(string, fmt, ap);
	*(string->bufpos) = 0;

	return rv;
}
