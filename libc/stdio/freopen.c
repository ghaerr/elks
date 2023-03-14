#include <stdio.h>

FILE *
freopen(const char *file, const char *mode, FILE *fp)
{
   return __fopen(file, -1, fp, mode);
}
