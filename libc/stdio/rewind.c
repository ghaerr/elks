#include <stdio.h>

void
rewind(FILE *fp)
{
    fseek(fp, (long)0, SEEK_SET);
    clearerr(fp);
}
