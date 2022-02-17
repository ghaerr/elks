#ifdef L_vscanf
#include <stdio.h>

int
vscanf(__const char *fmt, va_list ap)
{
  return vfscanf(stdin, fmt, ap);
}
#endif
