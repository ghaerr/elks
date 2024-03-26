#ifndef __UTIME_H
#define __UTIME_H

#include <sys/types.h>

struct utimbuf {
	time_t actime;
	time_t modtime;
};

int utime(const char *__filename, struct utimbuf *__utimebuf);

#endif

