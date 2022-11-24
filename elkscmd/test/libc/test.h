#ifndef ELKS_LIBC_TEST_H
#define ELKS_LIBC_TEST_H

#include <string.h>

extern int test_verbose;
extern int tests_fail;
extern const char *test_name;

void test_fail(const char *file, int line, const char *fmt, ...);
void test_info(const char *file, int line, const char *fmt, ...);
#define TEST_INFO(fmt,...) test_info(__FILE__, __LINE__, fmt, __VA_ARGS__)
#define TEST_CASE(T) \
	void test_##T() { \
		test_name = __func__; \
		if (test_verbose) TEST_INFO("starting%c", '\n'); \
		void _test_##T(); \
		_test_##T(); \
	} \
	void _test_##T()
#define TEST_FAIL(fmt,...) test_fail(__FILE__, __LINE__, fmt, __VA_ARGS__)
#define TEST_FAIL_STR(A,E) test_fail(__FILE__, __LINE__, "got '%s' expected '%s'\n", A, E)
#define TEST_FAIL_LONG(A,E) test_fail(__FILE__, __LINE__, "got '%ld' expected '%ld'\n", A, E)
#define TEST_EQUAL_STR(A,E) if (strcmp(A, E) != 0) TEST_FAIL_STR(A,E)

struct timeval;

int timeval_cmp(struct timeval *a, struct timeval *b);
int timeval_sub(struct timeval *a, struct timeval *b, struct timeval *diff);
void timeval_norm(struct timeval *a);
void timeval_add(struct timeval *a, struct timeval *b);

#endif
