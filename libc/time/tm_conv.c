
#if 0
#include <time.h>

/* This is a translation from ALGOL in Collected Algorithms of CACM. */
/* Copied from Algorithm 199, Author: Robert G. Tantzen */

void
__tm_conv(tmbuf, timep, offset)
struct tm *tmbuf;
time_t *timep;
time_t offset;
{
static int   moffset[] =
   {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

   long s;
   long  j, d, m, y;

   offset += *timep;

   tmbuf->tm_isdst = 0;		/* Someone else can set this */

   j = offset / 86400L + 719469;
   s = offset % 86400L;

   if( s < 0 ) { s += 86400L; j--; }

   tmbuf->tm_sec = s % 60;
   tmbuf->tm_min = (s / 60) % 60;
   tmbuf->tm_hour = s / 3600;

   tmbuf->tm_wday = (j+2) % 7;

   /*
    * Julian date converter. Takes a julian date (the number of days since
    * some distant epoch or other), and fills tmbuf.
    */

   y = (4L * j - 1L) / 146097L;
   j = 4L * j - 1L - 146097L * y;
   d = j / 4L;
   j = (4L * d + 3L) / 1461L;
   d = 4L * d + 3L - 1461L * j;
   d = (d + 4L) / 4L;
   m = (5L * d - 3L) / 153L;
   d = 5L * d - 3 - 153L * m;
   d = (d + 5L) / 5L;
   y = 100L * y + j;
   if (m < 10)
      m += 2;
   else
   {
      m -= 10;
      ++y;
   }

   tmbuf->tm_year = y - 1900;
   tmbuf->tm_mon = m;
   tmbuf->tm_mday = d;

   tmbuf->tm_yday = d + moffset[m];
   if (m > 1 && ((y) % 4 == 0 && ((y) % 100 != 0 || (y) % 400 == 0)))
      tmbuf->tm_yday++;
}

#else

/* This is adapted from glibc */
/* Copyright (C) 1991, 1993 Free Software Foundation, Inc */

#define SECS_PER_HOUR 3600L
#define SECS_PER_DAY  86400L

#include <time.h>

static const unsigned short int __mon_lengths[2][12] =
  {
    /* Normal years.  */
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    /* Leap years.  */
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
  };


void
__tm_conv(tmbuf, t, offset)
struct tm *tmbuf;
time_t *t;
time_t offset;
{
  long days, rem;
  register int y;
  register unsigned short int *ip;

  days = *t / SECS_PER_DAY;
  rem = *t % SECS_PER_DAY;
  rem += offset;
  while (rem < 0)
    {
      rem += SECS_PER_DAY;
      --days;
    }
  while (rem >= SECS_PER_DAY)
    {
      rem -= SECS_PER_DAY;
      ++days;
    }
  tmbuf->tm_hour = rem / SECS_PER_HOUR;
  rem %= SECS_PER_HOUR;
  tmbuf->tm_min = rem / 60;
  tmbuf->tm_sec = rem % 60;
  /* January 1, 1970 was a Thursday.  */
  tmbuf->tm_wday = (4 + days) % 7;
  if (tmbuf->tm_wday < 0)
    tmbuf->tm_wday += 7;
  y = 1970;
  while (days >= (rem = __isleap(y) ? 366 : 365))
    {
      ++y;
      days -= rem;
    }
  while (days < 0)
    {
      --y;
      days += __isleap(y) ? 366 : 365;
    }
  tmbuf->tm_year = y - 1900;
  tmbuf->tm_yday = days;
  ip = __mon_lengths[__isleap(y)];
  for (y = 0; days >= ip[y]; ++y)
    days -= ip[y];
  tmbuf->tm_mon = y;
  tmbuf->tm_mday = days + 1;
  tmbuf->tm_isdst = -1;
}

#endif
