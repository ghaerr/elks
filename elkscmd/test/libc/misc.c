#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

TEST_CASE(misc_basename)
{
	struct {
		const char *path;
		const char *base;
	} tests[] = {
		{ "", "." },
		{ "/usr/lib", "lib" },
		{ "/usr/", "usr" },
		{ "usr/", "usr" },
		{ "/", "/" },
		{ "///", "/" },
		{ "//usr//lib//", "lib" },
		{ ".", "." },
		{ "..", ".." },
		{ NULL, NULL }
	};

	for (int i = 0; tests[i].path; ++i) {
		char *path = strdup(tests[i].path);
		char *base = basename(path);
		TEST_EQUAL_STR(base, tests[i].base);
		free(path);
	}
}

TEST_CASE(misc_dirname)
{
	struct {
		const char *path;
		const char *dir;
	} tests[] = {
		{ "", "." },
		{ "/usr/lib/", "/usr" },
		{ "/usr/lib", "/usr" },
		{ "/usr/", "/" },
		{ "usr/", "." },
		{ "/", "/" },
		{ "///", "/" },
		{ "//usr//lib//", "/usr" },
		{ ".", "." },
		{ "..", "." },
		{ NULL, NULL }
	};

	for (int i = 0; tests[i].path; ++i) {
		char *path = strdup(tests[i].path);
		char *dir = dirname(path);
		TEST_EQUAL_STR(dir, tests[i].dir);
		free(path);
	}
}

#define TEST(r, f, x, m) ( \
	errno = 0, msg = #f, ((r) = (f)) == (x) || \
	(TEST_FAIL("%s failed (" m ")\n", #f, r, x), 0) )

#define TEST2(r, f, x, m) ( \
	((r) = (f)) == (x) || \
	(TEST_FAIL("%s failed (" m ")\n", msg, r, x), 0) )

TEST_CASE(misc_strtol)
{
	int i;
	long l;
	unsigned long ul;
	char *msg = "";
	char *s, *c;

	assert(sizeof(long) == 4);

	// test cases adapted from musl's libc-test
	TEST(l, strtol(s="2147483648", &c, 0), 2147483647L, "uncaught overflow %ld != %ld");
	TEST2(i, c-s, 10, "wrong final position %d != %d");
	TEST2(i, errno, ERANGE, "missing errno %d != %d");
	TEST(l, strtol(s="-2147483649", &c, 0), -2147483647L-1, "uncaught overflow %ld != %ld");
	TEST2(i, c-s, 11, "wrong final position %d != %d");
	TEST2(i, errno, ERANGE, "missing errno %d != %d");
	TEST(ul, strtoul(s="4294967296", &c, 0), 4294967295UL, "uncaught overflow %lu != %lu");
	TEST2(i, c-s, 10, "wrong final position %d != %d");
	TEST2(i, errno, ERANGE, "missing errno %d != %d");
	TEST(ul, strtoul(s="-1", &c, 0), -1UL, "rejected negative %lu != %lu");
	TEST2(i, c-s, 2, "wrong final position %d != %d");
	TEST2(i, errno, 0, "spurious errno %d != %d");
	TEST(ul, strtoul(s="-2", &c, 0), -2UL, "rejected negative %lu != %lu");
	TEST2(i, c-s, 2, "wrong final position %d != %d");
	TEST2(i, errno, 0, "spurious errno %d != %d");
	TEST(ul, strtoul(s="-2147483648", &c, 0), -2147483648UL, "rejected negative %lu != %lu");
	TEST2(i, c-s, 11, "wrong final position %d != %d");
	TEST2(i, errno, 0, "spurious errno %d != %d");
	TEST(ul, strtoul(s="-2147483649", &c, 0), -2147483649UL, "rejected negative %lu != %lu");
	TEST2(i, c-s, 11, "wrong final position %d != %d");
	TEST2(i, errno, 0, "spurious errno %d != %d");
	TEST(ul, strtoul(s="-4294967296", &c, 0), 4294967295UL, "uncaught negative overflow %lu != %lu");
	TEST2(i, c-s, 11, "wrong final position %d != %d");
	TEST2(i, errno, ERANGE, "spurious errno %d != %d");
}
