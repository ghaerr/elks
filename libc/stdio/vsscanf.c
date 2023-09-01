#ifdef L_vsscanf
#include <stdio.h>

int vsscanf(char *sp, const char *fmt, va_list ap)
{
	static FILE string[1] =
	{
		{
			0,
			(char *)(unsigned)-1,
			0,
			0,
			(char *)(unsigned)-1,
			-1,
			_IOFBF | __MODE_READ
		}
	};

	string->bufpos = sp;
	return vfscanf(string, fmt, ap);
}
#endif
