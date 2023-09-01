#ifdef L_vprintf
#include <stdio.h>

int
vprintf(const char * fmt, va_list ap)
{
  return vfprintf(stdout, fmt, ap);
}
#endif
