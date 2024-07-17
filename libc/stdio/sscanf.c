#include <stdio.h>

int
sscanf(const char *str, const char *format, ...)
{
	static FILE string[1] =
	{
		{
			0,
			(unsigned char *)(unsigned)-1,
			0,
			0,
			(unsigned char *)(unsigned)-1,
			-1,
			_IOFBF | __MODE_READ
		}
	};

	va_list ptr;
	int rv;

	va_start(ptr, format);
	string->bufpos = (unsigned char *)str;
	rv = vfscanf(string, (char *)format, ptr);
	va_end(ptr);

	return rv;
}
