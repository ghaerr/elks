#include <stdio.h>

int
printf(const char * fmt, ...)
{
  va_list ptr;
  int rv;

  va_start(ptr, fmt);
  rv = vfprintf(stdout,fmt,ptr);
  va_end(ptr);

  return rv;
}
