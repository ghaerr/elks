#include <stdio.h>

int
fprintf(FILE * fp, const char * fmt, ...)
{
  va_list ptr;
  int rv;

  va_start(ptr, fmt);
  rv = vfprintf(fp,fmt,ptr);
  va_end(ptr);

  return rv;
}
