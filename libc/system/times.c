#ifdef L_times
#include <time.h>

clock_t times(struct tms * buf)
{
   long rv;
   __times(buf, &rv);
   return rv;
}
#endif
