
#include <time.h>

extern void __tm_conv();

struct tm *
gmtime(timep)
__const time_t * timep;
{
   static struct tm tmb;

   __tm_conv(&tmb, timep, 0L);

   return &tmb;
}
