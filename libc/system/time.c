#include <sys/time.h>

time_t
time(time_t *where)
{
   struct timeval rv;

   if(gettimeofday(&rv, (void*)0) < 0 )
      return -1;

   if(where)
      *where = rv.tv_sec;

   return rv.tv_sec;
}
