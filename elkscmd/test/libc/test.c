#include <stdio.h>
#include <string.h>
#include <sys/time.h>

int timeval_cmp(struct timeval *a, struct timeval *b)
{
	if (a->tv_sec == b->tv_sec && a->tv_usec == b->tv_usec)
		return 0;
	if (a->tv_sec > b->tv_sec ||
		(a->tv_sec == b->tv_sec && a->tv_usec > b->tv_usec))
		return 1;
	return -1;
}

int timeval_sub(struct timeval *a, struct timeval *b, struct timeval *diff)
{
	if (a->tv_usec < b->tv_usec) {
		long sec = (b->tv_usec - a->tv_usec) / 1000000L + 1;
		b->tv_usec -= 1000000L * sec;
		b->tv_sec += sec;
	}
	if (a->tv_usec - b->tv_usec > 1000000L) {
		long sec = (a->tv_usec - b->tv_usec) / 1000000L;
		b->tv_usec += 1000000L * sec;
		b->tv_sec -= sec;
	}

	diff->tv_sec = a->tv_sec - b->tv_sec;
	diff->tv_usec = a->tv_usec - b->tv_usec;

	return a->tv_sec < b->tv_sec;
}

void timeval_norm(struct timeval *a)
{
	while (a->tv_usec > 1000000L) {
		a->tv_sec++;
		a->tv_usec -= 1000000L;
	}
}

void timeval_add(struct timeval *a, struct timeval *b)
{
	a->tv_sec += b->tv_sec;
	a->tv_usec += b->tv_usec;
	timeval_norm(a);
}

int test_verbose = 0;
int tests_fail = 0;
const char *test_name = NULL;

void test_fail(const char *file, int line, const char *fmt, ...)
{
	++tests_fail;
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "%s failed (%s:%d): ", test_name, file, line);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void test_info(const char *file, int line, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "%s info (%s:%d): ", test_name, file, line);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

int main(int argc, char **argv)
{
	if (argc > 1 && !strcmp(argv[1], "-v"))
		test_verbose = 1;

	test_error_strerror();
	test_misc_basename();
	test_misc_dirname();
	test_misc_strtol();
	test_stdio_init();
	test_time_gettimeofday();
	test_time_gmtime();
	test_time_mktime();

	return !!tests_fail;
}
