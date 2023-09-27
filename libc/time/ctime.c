#include <time.h>
#include <sys/time.h>

char *
ctime(const time_t *timep)
{
  time_t offt;
  struct tm tmb;
  struct timezone tz;
  static char cbuf[26];
  
  gettimeofday((void*)0, &tz);
  
  if (!_tz_is_set)
	tzset();
  tz.tz_minuteswest = timezone / 60;
  offt = -tz.tz_minuteswest*60L;
  
  /* tmb.tm_isdst = ? */
  __tm_conv(&tmb, timep, offt);
  
  __asctime(cbuf, &tmb);
  
  return cbuf;
}
