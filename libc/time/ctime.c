
#include <time.h>
#include <sys/time.h>

extern void __tm_conv();
extern void __asctime();

char *
ctime(timep)
__const time_t * timep;
{
  static char cbuf[26];
  struct tm tmb;
  struct timezone tz;
  time_t offt;
  
  gettimeofday((void*)0, &tz);
  
  offt = -tz.tz_minuteswest*60L;
  
  /* tmb.tm_isdst = ? */
  __tm_conv(&tmb, timep, offt);
  
  __asctime(cbuf, &tmb);
  
  return cbuf;
}
