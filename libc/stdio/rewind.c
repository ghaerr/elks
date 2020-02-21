#ifdef L_rewind
#include <stdio.h>

void
rewind(FILE *fp)
{
   fseek(fp, (long)0, 0);
   clearerr(fp);
}
#endif
