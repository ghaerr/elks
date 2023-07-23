/* Copyright 2023 Chuck Coffing <clc@alum.mit.edu>
 * SPDX-License-Identifier: 0BSD
 */

#ifndef ELKS_TESTLIB_H
#define ELKS_TESTLIB_H

/* pre run configuration */
extern int testlib_verbose;
extern int testlib_forkTest;
extern int testlib_debugOnFail;
extern int testlib_abortOnFail;
extern unsigned int testlib_seed;

/* post run stats */
extern unsigned int testlib_assertions;
extern unsigned int testlib_assertionFails;
extern unsigned int testlib_tests;
extern unsigned int testlib_testFails;

/* controlling the test run */
typedef void (*testfn_t)(void);
void testlib_runTestCases(testfn_t *start, testfn_t *end);
int testlib_report();
void testlib_debugTrap();

/* test helpers */
extern const char *test_name;

int testlib_getErrno();
void testlib_setErrno(int e);
const char *testlib_strerror();

int testlib_strequals(const void *s1, const void *s2);
int testlib_strnequals(const void *s1, const void *s2, int n);

struct timeval;
int testlib_tvEq(struct timeval *a, struct timeval *b);
void testlib_tvAdd(struct timeval *a, struct timeval *b);
int testlib_tvSub(struct timeval *a, struct timeval *b, struct timeval *diff);
void testlib_tvNormalize(struct timeval *a);

void *testlib_malloc(unsigned int size);
char *testlib_strdup(const char *s);

void testlib_capture(const char *key, const char *value);
void testlib_showInfo(const char *file, int line, const char *fmt, ...);
void testlist_showErrorFmt(const char *file, int line, const char *func,
                           const char *fmt, ...);
void testlib_showError(const char *file, int line, const char *func,
                       const char *kind, const char *expr, const char *v1,
                       const char *sym, const char *v2);

void testlib_onFail(int isFatal);

void testlib_abort(const char *fmt, ...);

#if defined(CP437)
#define SYM_EQ "\360"
#define SYM_NE "!="
#define SYM_GT ">"
#define SYM_GE "\362"
#define SYM_LT "<"
#define SYM_LE "\363"
#elif defined(ENABLE_UTF8)
#define SYM_EQ "≡"
#define SYM_NE "≠"
#define SYM_GT ">"
#define SYM_GE "≥"
#define SYM_LT "<"
#define SYM_LE "≤"
#else
#define SYM_EQ "=="
#define SYM_NE "!="
#define SYM_GT ">"
#define SYM_GE ">="
#define SYM_LT "<"
#define SYM_LE "<="
#endif

#define FILIFU __FILE__, __LINE__, __FUNCTION__,
#define FILIFU_ARGS const char *file, int line, const char *func,

#define ASSERT_TRUE(X) _ETL_BINOP(X, ==, 1, #X, "", "", 1)
#define ASSERT_FALSE(X) _ETL_BINOP(X, ==, 0, #X, "", "", 1)
#define ASSERT_EQ(GOT, WANT) _ETL_BINOP(GOT, ==, WANT, #GOT, SYM_EQ, #WANT, 1)
#define ASSERT_EQ_P(GOT, WANT) \
    _ETL_BINOP_P(GOT, ==, WANT, #GOT, SYM_EQ, #WANT, 1)
#define ASSERT_EQ_F(GOT, WANT) \
    _ETL_BINOP_F(GOT, ==, WANT, #GOT, SYM_EQ, #WANT, 1)
#define ASSERT_NE(GOT, WANT) _ETL_BINOP(GOT, !=, WANT, #GOT, SYM_NE, #WANT, 1)
#define ASSERT_NE_P(GOT, WANT) \
    _ETL_BINOP_P(GOT, !=, WANT, #GOT, SYM_EQ, #WANT, 1)
#define ASSERT_GT(X, C) _ETL_BINOP(X, >, C, #X, SYM_GT, #C, 1)
#define ASSERT_GE(X, C) _ETL_BINOP(X, >=, C, #X, SYM_GE, #C, 1)
#define ASSERT_LT(X, C) _ETL_BINOP(X, <, C, #X, SYM_LT, #C, 1)
#define ASSERT_LE(X, C) _ETL_BINOP(X, <=, C, #X, SYM_LE, #C, 1)
#define ASSERT_BETWEEN(GOT, BEG, END) \
    assertBetween(FILIFU GOT, BEG, END, #BEG " <= " #GOT " <= " #END, 1)
#define ASSERT_STREQ(GOT, WANT) testStringEquals(FILIFU GOT, WANT, #GOT, 1)
#define ASSERT_STRNE(GOT, NOPE) testStringNotEquals(FILIFU GOT, NOPE, #GOT, 1)
#define ASSERT_STREQN(GOT, WANT, N) testStrNEquals(FILIFU GOT, WANT, N, #GOT, 1)
#define ASSERT_STRNEN(GOT, NOPE, N) \
    assertStrnNotEquals(FILIFU GOT, NOPE, N, #GOT, 1)
#define ASSERT_STRCASEEQ(GOT, WANT) \
    assertStringCaseEquals(FILIFU GOT, WANT, #GOT, 1)
#define ASSERT_STRCASENE(GOT, NOPE) \
    assertStringCaseNotEquals(FILIFU GOT, NOPE, #GOT, 1)
#define ASSERT_STRNCASEEQ(GOT, WANT, N) \
    assertStrnCaseEquals(FILIFU GOT, WANT, N, #GOT, 1)
#define ASSERT_STRNCASENE(GOT, NOPE, N) \
    assertStrnCaseNotEquals(FILIFU GOT, NOPE, N, #GOT, 1)
#define ASSERT_STARTSWITH(GOT, PREFIX) \
    assertStartsWith(FILIFU GOT, PREFIX, #GOT, 1)
#define ASSERT_ENDSWITH(GOT, SUFFIX) assertEndsWith(FILIFU GOT, SUFFIX, #GOT, 1)
#define ASSERT_IN(GOT, NEEDLE) assertContains(FILIFU GOT, NEEDLE, #GOT, 1)

#define EXPECT_TRUE(X) _ETL_BINOP(X, ==, 1, #X, "", "", 0)
#define EXPECT_FALSE(X) _ETL_BINOP(X, ==, 0, #X, "", "", 0)
#define EXPECT_EQ(GOT, WANT) _ETL_BINOP(GOT, ==, WANT, #GOT, SYM_EQ, #WANT, 0)
#define EXPECT_EQ_P(GOT, WANT) \
    _ETL_BINOP_P(GOT, ==, WANT, #GOT, SYM_EQ, #WANT, 0)
#define EXPECT_NE(GOT, WANT) _ETL_BINOP(GOT, !=, WANT, #GOT, SYM_EQ, #WANT, 0)
#define EXPECT_NE_P(GOT, WANT) \
    _ETL_BINOP_P(GOT, !=, WANT, #GOT, SYM_EQ, #WANT, 0)
#define EXPECT_GT(X, C) _ETL_BINOP(X, >, C, #X, SYM_GT, #C, 0)
#define EXPECT_GE(X, C) _ETL_BINOP(X, >=, C, #X, SYM_GE, #C, 0)
#define EXPECT_LT(X, C) _ETL_BINOP(X, <, C, #X, SYM_LT, #C, 0)
#define EXPECT_LE(X, C) _ETL_BINOP(X, <=, C, #X, SYM_LE, #C, 0)
#define EXPECT_BETWEEN(GOT, BEG, END) \
    assertBetween(FILIFU GOT, BEG, END, #BEG " <= " #GOT " <= " #END, 0)
#define EXPECT_STREQ(GOT, WANT) testStringEquals(FILIFU GOT, WANT, #GOT, 0)
#define EXPECT_STRNE(GOT, NOPE) testStringNotEquals(FILIFU GOT, NOPE, #GOT, 0)
#define EXPECT_STREQN(GOT, WANT, N) testStrNEquals(FILIFU GOT, WANT, N, #GOT, 0)
#define EXPECT_STRNEN(GOT, NOPE, N) \
    assertStrnNotEquals(FILIFU GOT, NOPE, N, #GOT, 0)
#define EXPECT_STRCASEEQ(GOT, WANT) \
    assertStringCaseEquals(FILIFU GOT, WANT, #GOT, 0)
#define EXPECT_STRCASENE(GOT, NOPE) \
    assertStringCaseNotEquals(FILIFU GOT, NOPE, #GOT, 0)
#define EXPECT_STRNCASEEQ(GOT, WANT, N) \
    assertStrnCaseEquals(FILIFU GOT, WANT, N, #GOT, 0)
#define EXPECT_STRNCASENE(GOT, NOPE, N) \
    assertStrnCaseNotEquals(FILIFU GOT, NOPE, N, #GOT, 0)
#define EXPECT_STARTSWITH(GOT, PREFIX) \
    assertStartsWith(FILIFU GOT, PREFIX, #GOT, 0)
#define EXPECT_ENDSWITH(GOT, SUFFIX) assertEndsWith(FILIFU GOT, SUFFIX, #GOT, 0)
#define EXPECT_IN(GOT, NEEDLE) assertContains(FILIFU GOT, NEEDLE, #GOT, 0)

#define ASSERT_SYS(GOT, WANT, WANTERRNO) \
    do { \
        errno = 0; \
        int Got = GOT; \
        int e = testlib_getErrno(); \
        _ETL_BINOP(Got, ==, WANT, #GOT, SYM_EQ, #WANT, 1); \
        _ETL_BINOP(e, ==, WANTERRNO, testlib_strerror(e), SYM_EQ, #WANTERRNO, \
                   1); \
        testlib_setErrno(e); \
    } while (0)

#define ASSERT_FAIL(FMT, ...) \
    do { \
        ++testlib_assertions; \
        testlist_showErrorFmt(FILIFU FMT, __VA_ARGS__); \
        testlib_onFail(1); \
    } while (0)

static inline void
testStringEquals(FILIFU_ARGS const void *got, const void *want,
                 const char *gotExpr, int isfatal)
{
    ++testlib_assertions;
    if (testlib_strequals(got, want))
        return;
    testlib_showError(file, line, func, "cstring equality", gotExpr, got,
                      SYM_EQ, want);
    testlib_onFail(isfatal);
}

static inline void
testStringNotEquals(FILIFU_ARGS const void *got, const void *nope,
                    const char *gotExpr, int isfatal)
{
    ++testlib_assertions;
    if (!testlib_strequals(got, nope))
        return;
    testlib_showError(file, line, func, "cstring inequality", gotExpr, got,
                      SYM_NE, nope);
    testlib_onFail(isfatal);
}

static inline void
testStrNEquals(FILIFU_ARGS const void *got, const void *want, int n,
               const char *gotExpr, int isfatal)
{
    ++testlib_assertions;
    if (testlib_strnequals(got, want, n))
        return;
    testlib_showError(file, line, func, "cstring equality", gotExpr, got,
                      SYM_EQ, want);
    testlib_onFail(isfatal);
}

#define _ETL_BINOP(GOT, OP, WANT, GOTEXPR, OPSTR, WANTEXPR, ISFATAL) \
    do { \
        long got = (long)(GOT); \
        long want = (long)(WANT); \
        ++testlib_assertions; \
        if (!(got OP want)) { \
            testlist_showErrorFmt(FILIFU "\texpr %s %s %s\n" \
                                         "\tgot  %ld\n" \
                                         "\t%s %s %ld\n", \
                                  GOTEXPR, OPSTR, WANTEXPR, got, \
                                  ISFATAL ? "need" : "want", OPSTR, want); \
            testlib_onFail(ISFATAL); \
        } \
    } while (0)

#define _ETL_BINOP_P(GOT, OP, WANT, GOTEXPR, OPSTR, WANTEXPR, ISFATAL) \
    do { \
        void *got = (void *)(GOT); \
        void *want = (void *)(WANT); \
        ++testlib_assertions; \
        if (!(got OP want)) { \
            testlist_showErrorFmt(FILIFU "\texpr %s\n" \
                                         "\tgot  %p\n" \
                                         "\t%s %s %p\n", \
                                  GOTEXPR " " OPSTR " " WANTEXPR, got, \
                                  ISFATAL ? "need" : "want", OPSTR, want); \
            testlib_onFail(ISFATAL); \
        } \
    } while (0)

#define _ETL_BINOP_F(GOT, OP, WANT, GOTEXPR, OPSTR, WANTEXPR, ISFATAL) \
    do { \
        double got = (double)(GOT); \
        double want = (double)(WANT); \
        ++testlib_assertions; \
        if (!(got OP want)) { \
            testlist_showErrorFmt(FILIFU "\texpr %s\n" \
                                         "\tgot  %f\n" \
                                         "\t%s %s %f\n", \
                                  GOTEXPR " " OPSTR " " WANTEXPR, got, \
                                  ISFATAL ? "need" : "want", OPSTR, want); \
            testlib_onFail(ISFATAL); \
        } \
    } while (0)

#define CAPTURE(K, V) testlib_capture(K, V);

#define TEST_INFO(FMT, ...) \
    testlib_showInfo(__FILE__, __LINE__, FMT, __VA_ARGS__)

#define TEST_CASE(T) \
    void test_##T() \
    { \
        test_name = __func__; \
        if (testlib_verbose) \
            TEST_INFO("%s '%s'\n", "starting", test_name); \
        void _test_##T(); \
        _test_##T(); \
    } \
    void _test_##T()

#endif
