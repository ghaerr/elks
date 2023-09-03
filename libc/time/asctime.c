#include <time.h>

char *
asctime(const struct tm *timeptr)
{
   static char timebuf[26];

   if( timeptr == 0 ) return 0;
   __asctime(timebuf, timeptr);
   return timebuf;
}
