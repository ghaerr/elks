#ifndef _SYS_TIME_H
#define _SYS_TIME_H

#include <time.h>

int gettimeofday (struct timeval * restrict tp, void * restrict tzp);
int settimeofday (const struct timeval *tp, const struct timezone *tzp);

#endif
