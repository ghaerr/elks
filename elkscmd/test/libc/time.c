#include <stdio.h>
#include <stdlib.h>
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
		puts("incorrect tm");
		puts("\tactual   "); puts(tm_str(a));
		puts("\texpected "); puts(tm_str(b));
		fail++;
	}
}

void check_time(time_t a, time_t b)
{
	if (a != b) {
		puts("incorrect time_t");
		printf("\tactual   %lu\n", a);
		printf("\texpected %lu\n", b);
		fail++;
	}
}

void test_gmtime()
{
	struct tm *tm;
	int i;

	for (i = 0; i < N_TM_TESTS; ++i) {
		tm = gmtime(&tm_tests[i].offset);
		check_tm(tm, &tm_tests[i].tm);
	}
}

/* FIXME currently fails */
void test_mktime()
{
	struct tm *tm;
	int i;

	for (i = 0; i < N_TM_TESTS; ++i) {
		time_t t;
		struct tm tm = tm_tests[i].tm;
		tm.tm_isdst = 0;
		t = mktime(&tm);
		check_time(t, tm_tests[i].offset);
	}
}
