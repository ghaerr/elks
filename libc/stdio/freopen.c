#include <stdio.h>

FILE *
freopen(char *file, char *mode, FILE *fp)
{
   return __fopen(file, -1, fp, mode);
}
