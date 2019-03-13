
#include <time.h>

#include <sys/time.h>

extern void __tm_conv();

struct tm * localtime (const time_t * timep)
{
   static struct tm tmb;
   struct timezone tz;
   time_t offt;

   gettimeofday((void*)0, &tz);

   offt = -tz.tz_minuteswest*60L;

   /* tmb.tm_isdst = ? */
   __tm_conv(&tmb, timep, offt);

   return &tmb;
}
