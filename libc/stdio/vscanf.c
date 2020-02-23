#ifdef L_vscanf
#include <stdio.g>

int
vscanf(__const char *fmt, va_list ap)
{
  return vfscanf(stdin, fmt, ap);
}
#endif
