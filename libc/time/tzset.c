#include <stdlib.h>
#include <sys/time.h>

int _tz_is_set;
long timezone;		/* seconds west of GMT*/

#define isnumber(c)		(((c) >= '0' && (c) <= '9') || (c) == '-')

void tzset(void)
{
	char *p = getenv("TZ");

	if (p) {
		while (*p && !isnumber(*p))
			p++;
		timezone = atoi(p) * 3600L;
	} else timezone = 0;
	_tz_is_set = 1;
}
