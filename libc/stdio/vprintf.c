#ifdef L_vprintf
#include <stdio.h>

int
vprintf(__const char * fmt, va_list ap)
{
  return vfprintf(stdout, fmt, ap);
}
#endif
