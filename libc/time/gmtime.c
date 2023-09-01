#include <time.h>

void __tm_conv(struct tm *tmbuf, const time_t *t, time_t offset);

struct tm *
gmtime(const time_t *timep)
{
   static struct tm tmb;

   __tm_conv(&tmb, timep, 0L);

   return &tmb;
}
