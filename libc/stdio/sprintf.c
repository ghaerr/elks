#include <stdio.h>

int
sprintf(char *sp, const char *fmt, ...)
{
  static FILE string[1] =
  {
    {
     0,
     0,
     (unsigned char *)(unsigned)-1,
     0,
     (unsigned char *)(unsigned)-1,
     -1,
     _IOFBF | __MODE_WRITE}
  };

  va_list ptr;
  int rv;

  va_start(ptr, fmt);
  string->bufpos = (unsigned char *)sp;
  rv = vfprintf(string, fmt, ptr);
  va_end(ptr);
  *(string->bufpos) = 0;

  return rv;
}
