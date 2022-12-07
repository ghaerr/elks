#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "test.h"

#define N_TM_TESTS 2
struct tm_map {
	time_t offset;
	struct tm tm;
} tm_tests[N_TM_TESTS] = {
	{          0, { 0, 0, 0, 1, 0, 70, 4, 0, -1 } },
	{ 1668875025, { 45, 23, 16, 19, 10, 122, 6, 322, -1 } },
	/*{           , { 8, 14, 3, 19, 0, 138, 2, 18, -1 } },*/
};

int tm_cmp(struct tm *a, struct tm *b)
{
	return  a->tm_sec   != b->tm_sec  ||
		a->tm_min   != b->tm_min  ||
		a->tm_hour  != b->tm_hour ||
		a->tm_mday  != b->tm_mday ||
		a->tm_mon   != b->tm_mon  ||
		a->tm_year  != b->tm_year ||
		a->tm_wday  != b->tm_wday ||
		a->tm_yday  != b->tm_yday ||
		a->tm_isdst != b->tm_isdst;
}

char *tm_str(struct tm *tm)
{
	static char s[64];
	sprintf(s, "s=%d m=%d h=%d mday=%d mon=%d year=%d wday=%d yday=%d isdst=%d",
		tm->tm_sec, tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_mon,
		tm->tm_year, tm->tm_wday, tm->tm_yday, tm->tm_isdst);
	return s;
}

void check_tm(struct tm *a, struct tm *b)
{
	if (tm_cmp(a, b)) {
		char *a_str = strdup(tm_str(a));
		TEST_FAIL_STR(a_str, tm_str(b));
		free(a_str);
	}
}

void check_time(time_t a, time_t b)
{
	if (a != b) {
		TEST_FAIL_LONG(a, b);
	}
}

TEST_CASE(time_gmtime)
{
	struct tm *tm;
	int i;

	for (i = 0; i < N_TM_TESTS; ++i) {
		tm = gmtime(&tm_tests[i].offset);
		check_tm(tm, &tm_tests[i].tm);
	}
}

/* FIXME currently fails */
TEST_CASE(time_mktime)
{
	int i;

	for (i = 0; i < N_TM_TESTS; ++i) {
		time_t t;
		struct tm tm = tm_tests[i].tm;
		tm.tm_isdst = 0;
		t = mktime(&tm);
		check_time(t, tm_tests[i].offset);
	}
}

TEST_CASE(time_gettimeofday)
{
	const int N = 100;
	struct timeval tv[N];
	for (int i = 0; i < N; ++i) {
		if (gettimeofday(&tv[i], NULL) < 0) {
			perror("gettimeofday");
			tests_fail++;
			return;
		}
	}

	struct timeval tot = { 0, 0 };
	struct timeval max = { 0, 0 };
	for (int i = 1; i < N; ++i) {
		struct timeval diff;
		/* possible for leap seconds? */
		if (tv[i].tv_usec > 1000000L) {
			fputs("gettimeofday: usec was too large\n", stderr);
			tests_fail++;
			return;
		}
		if (timeval_sub(&tv[i], &tv[i-1], &diff)) {
			fputs("gettimeofday: time went backward\n", stderr);
			tests_fail++;
			return;
		}
		if (diff.tv_sec > 2) {
			fputs("gettimeofday: time jumped forward\n", stderr);
			tests_fail++;
			return;
		}
		if (timeval_cmp(&diff, &max) > 0)
			max = diff;
		timeval_add(&tot, &diff);
	}

	struct timeval avg;
	avg.tv_sec = tot.tv_sec / N;
	avg.tv_usec = tot.tv_sec % N * 1000000L + tot.tv_usec / N;
	timeval_norm(&avg);
	TEST_INFO("gettimeofday: %ld.%06ld sec/call overhead\n", avg.tv_sec, avg.tv_usec);
}
