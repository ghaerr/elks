/* Copyright 2023 Chuck Coffing <clc@alum.mit.edu>
 * SPDX-License-Identifier: 0BSD
 */

#include "testlib.h"

#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int testlib_verbose;
int testlib_forkTest;
int testlib_debugOnFail;
int testlib_abortOnFail;
unsigned int testlib_seed;
unsigned int testlib_assertions;
unsigned int testlib_assertionFails;
unsigned int testlib_tests;
unsigned int testlib_testFails;
const char *test_name;

static jmp_buf env;
static void *autoFree;
static const char *captureKey;
static const char *captureValue;

int
testlib_getErrno()
{
    return errno;
}

void
testlib_setErrno(int e)
{
    errno = e;
}

const char *
testlib_strerror()
{
    return strerror(errno);
}

int
testlib_strequals(const void *s1, const void *s2)
{
    if (s1 == s2)
        return 1;
    if (!s1 || !s2)
        return 0;
    return strcmp(s1, s2) == 0;
}

int
testlib_strnequals(const void *s1, const void *s2, int n)
{
    if (s1 == s2)
        return 1;
    if (!s1 || !s2)
        return 0;
    return strncmp(s1, s2, n) == 0;
}

int
testlib_tvEq(struct timeval *a, struct timeval *b)
{
    if (a->tv_sec == b->tv_sec && a->tv_usec == b->tv_usec)
        return 0;
    if (a->tv_sec > b->tv_sec ||
        (a->tv_sec == b->tv_sec && a->tv_usec > b->tv_usec))
        return 1;
    return -1;
}

void
testlib_tvAdd(struct timeval *a, struct timeval *b)
{
    a->tv_sec += b->tv_sec;
    a->tv_usec += b->tv_usec;
    testlib_tvNormalize(a);
}

int
testlib_tvSub(struct timeval *a, struct timeval *b, struct timeval *diff)
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

void
testlib_tvNormalize(struct timeval *a)
{
    while (a->tv_usec > 1000000L) {
        a->tv_sec++;
        a->tv_usec -= 1000000L;
    }
}

void
testlib_abort(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    abort();
}

void *
testlib_malloc(unsigned int size)
{
    char *p = malloc(sizeof(void *) + size);
    if (p == NULL)
        testlib_abort("%s failed: %s\n", "malloc", strerror(errno));
    *(void **)p = autoFree;
    autoFree = p;
    return p + sizeof(void *);
}

char *
testlib_strdup(const char *s)
{
    char *p = strdup(s);
    if (p == NULL)
        testlib_abort("%s failed: %s\n", "strdup", strerror(errno));
    *(void **)p = autoFree;
    autoFree = p;
    return p + sizeof(void *);
}

void
testlib_capture(const char *key, const char *value)
{
    captureKey = key;
    captureValue = value;
}

void
testlib_showInfo(const char *file, int line, const char *fmt, ...)
{
    if (!testlib_verbose)
        return;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stdout, "%s:%d: ", file, line);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
}

void
testlist_showErrorFmt(const char *file, int line, const char *func,
                      const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "Test '%s' failed in %s() %s:%d:\n", test_name, func, file,
            line);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
}

void
testlib_showError(const char *file, int line, const char *func,
                  const char *kind, const char *expr, const char *v1,
                  const char *sym, const char *v2)
{
    fprintf(stderr,
            "Test '%s' failed %s in %s() %s:%d:\n"
            "\texpr %s\n"
            "\tgot  \"%s\"\n"
            "\tneed %s \"%s\"\n",
            test_name, kind, func, file, line, expr, v1 ? v1 : "(null)", sym,
            v2 ? v2 : "null");
}

void
testlib_debugTrap()
{
#ifdef ELKS
    asm("int3");
#else
#endif
}

void
testlib_onFail(int isFatal)
{
    if (captureKey)
        fprintf(stderr, "\tcapture: %s = \"%s\"\n", captureKey, captureValue);
    ++testlib_assertionFails;
    if (testlib_debugOnFail)
        testlib_debugTrap();
    if (isFatal)
        longjmp(env, 1);
}

static void
testlib_initEnv()
{
    putenv("TZ");
    tzset();
    srand(testlib_seed);
    errno = 0;
    captureKey = captureValue = NULL;
}

static void
testlib_deinitEnv()
{
    while (autoFree) {
        void *p = autoFree;
        autoFree = *(void **)autoFree;
        free(p);
    }
}

void
testlib_runTestCases(testfn_t *start, testfn_t *end)
{
    for (testfn_t *fn = start; fn != end; ++fn) {
        pid_t pid = -1;
        int failed = 0;

        if (testlib_forkTest) {
            pid = fork();
            if (pid == (pid_t)-1) {
                testlib_abort("%s failed: %s\n", "fork", strerror(errno));
            }
        }

        ++testlib_tests;
        if (pid == 0 || pid == (pid_t)-1) {
            unsigned int startAsserts = testlib_assertionFails;

            testlib_initEnv();

            if (setjmp(env) == 0) {
                (*fn)();

                if (testlib_assertionFails > startAsserts)
                    failed = 1;
            } else {
                failed = 1;
            }

            testlib_deinitEnv();

            if (pid == 0)
                exit(failed);
        }

        if (pid != 0 && pid != (pid_t)-1) {
            /* TODO update assertion count in parent process */
            if (waitpid(pid, &failed, 0) == (pid_t)-1) {
                testlib_abort("%s failed: %s\n", "waitpid", strerror(errno));
            }
        }

        if (failed) {
            ++testlib_testFails;
            if (testlib_abortOnFail)
                break;
        }
    }
}

int
testlib_report()
{
    fprintf(stderr, "\n%u / %u %s\n%u / %u %s\n", testlib_assertionFails,
            testlib_assertions, "assertions failed", testlib_testFails,
            testlib_tests, "tests failed");
    return !!testlib_assertionFails;
}
