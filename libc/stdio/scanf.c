#include "_stdio.h"

int
scanf(const char * fmt, ...)
{
  va_list ptr;
  int rv;

  va_strt(ptr, fmt);
  rv = vfscanf(stdin, (char *)fmt, ptr);
  va_end(ptr);

  return rv;
}
