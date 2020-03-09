#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include "lib.h"

/* our own happy mktime() replacement, with the following drawbacks: */
/*    doesn't check boundary conditions */
/*    doesn't set wday or yday */
/*    doesn't return the local time */
time_t utc_mktime(t)
struct tm *t;
{
	static int moffset[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
	time_t ret;

  /* calculate days from years */
	ret = t->tm_year - 1970;
	ret *= 365L;

  /* count leap days in preceding years */
	ret += ((t->tm_year -1969) >> 2);

  /* calculate days from months */
	ret += moffset[t->tm_mon];

  /* add in this year's leap day, if any */
   if (((t->tm_year & 3) == 0) && (t->tm_mon > 1)) {
		ret ++;
   }

  /* add in days in this month */
   ret += (t->tm_mday - 1);

  /* convert to hours */
	ret *= 24L;
   ret += t->tm_hour;

  /* convert to minutes */
   ret *= 60L;
   ret += t->tm_min;

  /* convert to seconds */
  	ret *= 60L;
   ret += t->tm_sec;

  /* return the result */
   return ret;
}

