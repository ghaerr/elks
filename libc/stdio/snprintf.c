#include <stdio.h>

int
snprintf(char *sp, size_t size, const char *fmt, ...)
{
  va_list ptr;
  int rv;

  va_start(ptr, fmt);
  rv = vsnprintf(sp, size, fmt, ptr);
  va_end(ptr);

  return rv;
}
