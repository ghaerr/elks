#if !defined(LIB_H)
#define	LIB_H

#include <sys/types.h>

#if defined(__cplusplus)
extern "C" {
#endif

void *
bsearch(const void * key, const void * base, size_t nmemb, size_t size,
	int (* compar)(const void *, const void *));

void strip_trailing_slashes(char *path);

time_t	utc_mktime(struct tm * t);

#if defined(__cplusplus)
}
#endif

#endif
