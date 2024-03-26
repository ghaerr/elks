#ifndef _SYS_TIMES_H
#define _SYS_TIMES_H

#include <time.h>

struct tms {
	clock_t tms_utime;
	clock_t tms_stime;
	clock_t tms_cutime;
	clock_t tms_cstime;
};

clock_t times (struct tms *tp);

#endif
