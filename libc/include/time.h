#ifndef __TIME_H
#define __TIME_H

#include <features.h>
#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>

typedef int64_t clock_t;

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

//extern char *tzname[2];
//extern int daylight;
extern long timezone;

time_t	time(time_t * __tp);
#ifndef __HAS_NO_FLOATS__
double  difftime(time_t __time2, time_t __time1);
#endif
time_t	mktime(struct tm * __tp);
char *	asctime(const struct tm * __tp);
char *	ctime(const time_t * __tp);
size_t	strftime(char * __s, size_t __smax, const char * __fmt, const struct tm * __tp);
void	tzset(void);

struct tm*	gmtime(const time_t *__tp);
struct tm*	localtime(const time_t * __tp);

#ifdef __LIBC__
extern int _tz_is_set;
void __tm_conv(struct tm *tmbuf, const time_t *timep, time_t offset);
void __asctime(char *buffer, const struct tm *ptm);
#endif

#endif
