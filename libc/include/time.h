#ifndef __TIME_H
#define __TIME_H

#include <sys/types.h>

typedef int64_t clock_t;

#define CLOCKS_PER_SEC	100
#define CLK_TCK         100     /* That must be the same as HZ ???? */

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

#define	__isleap(year)	((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))

/*extern char *tzname[2];
extern int daylight;*/
extern long timezone;

time_t time(time_t *tloc);
time_t mktime(struct tm *timeptr);
char * asctime(const struct tm *timeptr);
char * ctime(const time_t *clock);
struct tm *gmtime(const time_t *clock);
struct tm *localtime(const time_t *clock);
size_t	strftime(char *s, size_t maxsize, const char *format, const struct tm *tm);
void	tzset(void);

#ifdef __LIBC__
extern int _tz_is_set;
void __tm_conv(struct tm *tmbuf, const time_t *timep, time_t offset);
void __asctime(char *buffer, const struct tm *ptm);
#endif

#endif
