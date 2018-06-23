#ifndef __UTIME_H
#define __UTIME_H

#include <features.h>
#include <sys/types.h>

struct utimbuf {
	time_t actime;
	time_t modtime;
};

extern int utime __P ((char *__filename, struct utimbuf *__utimebuf));

#endif

