#ifndef __TIME_H
#define __TIME_H

#include <features.h>
#include <sys/types.h>
#include <stddef.h>

#ifndef _CLOCK_T
#define _CLOCK_T
typedef long clock_t;
#endif

#define CLOCKS_PER_SEC	100
#define CLK_TCK		100	/* That must be the same as HZ ???? */

struct timeval {
	long tv_sec;
	long tv_usec;
};

struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

struct timezone {
	int	tz_minuteswest;	/* minutes west of Greenwich */
	int	tz_dsttime;	/* type of dst correction */
};

#define	__isleap(year)	\
  ((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))

extern char *tzname[2];
extern int daylight;
extern long int timezone;

__BEGIN_DECLS

extern int	stime __P ((time_t* __tptr));

extern clock_t	clock __P ((void));
extern time_t	time __P ((time_t * __tp));
#ifndef __HAS_NO_FLOATS__
extern __CONSTVALUE double difftime __P ((time_t __time2,
					  time_t __time1)) __CONSTVALUE2;
#endif
extern time_t	mktime __P ((struct tm * __tp));

extern char *	asctime __P ((__const struct tm * __tp));
extern char *	ctime __P ((__const time_t * __tp));
extern size_t	strftime __P ((char * __s, size_t __smax,
			__const char * __fmt, __const struct tm * __tp));
extern void	tzset __P ((void));

extern struct tm*	gmtime __P ((__const time_t *__tp));
extern struct tm*	localtime __P ((__const time_t * __tp));

__END_DECLS

#endif
