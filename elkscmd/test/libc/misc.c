#include "testlib.h"

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
        ASSERT_STREQ(base, tests[i].base);
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
        /* TODO does not yet clean up slashes */
        { "//usr//lib//", "//usr" },
        { ".", "." },
        { "..", "." },
        { NULL, NULL }
    };

    for (int i = 0; tests[i].path; ++i) {
        char *path = strdup(tests[i].path);
        char *dir = dirname(path);
        ASSERT_STREQ(dir, tests[i].dir);
        free(path);
    }
}

#define TEST(r, f, x, m) \
    do { \
        errno = 0; \
        msg = #f; \
        if (((r) = (f)) != (x)) \
            ASSERT_FAIL("%s failed (" m ")\n", #f, r, x); \
    } while (0)

#define TEST2(r, f, x, m) \
    if (((r) = (f)) != (x)) ASSERT_FAIL("%s failed (" m ")\n", msg, r, x)

TEST_CASE(misc_strtol)
{
    int i;
    long l;
    unsigned long ul;
    char *msg = "";
    char *s, *c;

    assert(sizeof(long) == 4);

    /* test cases adapted from musl's libc-test */
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

TEST_CASE(misc_getopt)
{
    /* TODO */
}

TEST_CASE(misc_getcwd)
{
    char buf[PATH_MAX];
    char *p;

    p = getcwd(buf, 1);
    EXPECT_EQ(errno, ERANGE);
    EXPECT_EQ_P(p, NULL);

    memset(buf, 'X', sizeof(buf));
    p = getcwd(buf, sizeof(buf));
    EXPECT_EQ_P(p, buf);
    EXPECT_EQ(buf[0], '/');
    EXPECT_LT(strlen(buf), sizeof(buf)); /* is NULL terminated */
}

static void validateCrypt(const char *s)
{
    /* NULL terminated, "sane" length, printable ASCII, no \ : ; * ! */
    size_t len = strlen(s);
    EXPECT_GT(len, 10);
    EXPECT_LT(len, 64);
    for (size_t i = 0; i < len; ++i) {
        char c = s[i];
        if (c < 32 || strpbrk(s, "\\:;*!")) {
            ASSERT_FAIL("Invalid character 0x%02x\n", c);
        }
    }
}

TEST_CASE(misc_crypt)
{
    /* Implements TEA (tiny encryption algorithm) as documented here
     * http://www.ftp.cl.cam.ac.uk/ftp/papers/djw-rmn/djw-rmn-tea.html
     *
     * Hardcoded to require 2 bytes of salt, no error checking
     *
     * Can consume long passwords: current getpass() can return 128 chars
     */
    char salt[2];

    /* TODO crypt does not encode the salt which limits valid values;
     * does no error checking of valid values; verify this is correct
     */

    /* vary salt */
    salt[0] = 'a';
    salt[1] = 'b';
    char *p = crypt("secret", salt);
    char *p1 = testlib_strdup(p);
    validateCrypt(p1);
    salt[1] = 'c';
    p = crypt("secret", salt);
    validateCrypt(p);
    EXPECT_STRNE(p, p1);

    /* long password */
    char *secret = testlib_malloc(128);
    memset(secret, 'X', 127);
    secret[127] = 0;
    p = crypt(secret, salt);
    p1 = testlib_strdup(p);
    validateCrypt(p1);
    secret[126] = 'Y';
    p = crypt(secret, salt);
    validateCrypt(p);
    EXPECT_STRNE(p, p1);
}
