#ifndef _SYS_TIME_H
#define _SYS_TIME_H

#include <time.h>

#define ITIMER_REAL     0

struct  itimerval {
    struct  timeval it_interval;    /* timer interval */
    struct  timeval it_value;       /* current value */
};

int gettimeofday (struct timeval * restrict tp, void * restrict tzp);
int settimeofday (const struct timeval *tp, const struct timezone *tzp);

int setitimer(int which, const struct itimerval *value, struct itimerval *ovalue);

#endif
