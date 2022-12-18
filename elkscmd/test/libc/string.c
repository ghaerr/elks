#include "testlib.h"

#include <string.h>

TEST_CASE(string_strlen)
{
    EXPECT_EQ(strlen(""), 0);
    EXPECT_EQ(strlen("abc"), 3);
    EXPECT_EQ(strlen("\001\177\200\377\0a"), 4);
}

TEST_CASE(string_strcpy)
{
    char dst[4];

    memset(dst, 'X', sizeof(dst));
    EXPECT_TRUE(strcpy(dst, "") == dst);
    EXPECT_EQ(dst[0], '\0');
    EXPECT_EQ(dst[1], 'X');

    memset(dst, 'X', sizeof(dst));
    EXPECT_TRUE(strcpy(dst, "ab") == dst);
    EXPECT_EQ(dst[0], 'a');
    EXPECT_EQ(dst[1], 'b');
    EXPECT_EQ(dst[2], '\0');
    EXPECT_EQ(dst[3], 'X');
}

TEST_CASE(string_strncpy)
{
    char dst[4];

    memset(dst, 'X', sizeof(dst));
    EXPECT_TRUE(strncpy(dst, "ab", 1) == dst);
    EXPECT_EQ(dst[0], 'a');
    EXPECT_EQ(dst[1], 'X');

    memset(dst, 'X', sizeof(dst));
    EXPECT_TRUE(strncpy(dst, "ab", 4) == dst);
    EXPECT_EQ(dst[0], 'a');
    EXPECT_EQ(dst[1], 'b');
    EXPECT_EQ(dst[2], '\0');
    EXPECT_EQ(dst[3], '\0');  /* fills tail */
}

TEST_CASE(string_strcat)
{
    char dst[4];

    memset(dst, 'X', sizeof(dst));
    dst[0] = 0;
    strcat(dst, "1");
    EXPECT_STREQ(dst, "1");

    memset(dst, 'X', sizeof(dst));
    dst[0] = '1';
    dst[1] = 0;
    strcat(dst, "");
    EXPECT_STREQ(dst, "1");

    memset(dst, 'X', sizeof(dst));
    dst[0] = '1';
    dst[1] = 0;
    strcat(dst, "2");
    EXPECT_STREQ(dst, "12");
}

TEST_CASE(string_strncat)
{
    char dst[4];
    const char src[3] = { 'a', 'b', 0 };

    dst[0] = 'X'; dst[1] = 0; dst[2] = 'X'; dst[3] = 'X';
    strncat(dst, src, 1);
    EXPECT_STREQ(dst, "Xa");

    dst[0] = 'X'; dst[1] = 0; dst[2] = 'X'; dst[3] = 'X';
    strncat(dst, src, 2);
    EXPECT_STREQ(dst, "Xab");

    dst[0] = 'X'; dst[1] = 0; dst[2] = 'X'; dst[3] = 'X';
    strncat(dst, src, 3);
    EXPECT_STREQ(dst, "Xab");
}

TEST_CASE(string_strcmp)
{
    char a[4];
    char b[4];

    a[0] = b[0] = 0;
    EXPECT_EQ(strcmp(a, b), 0);

    strcpy(a, "a");
    strcpy(b, "b");
    EXPECT_LT(strcmp(a, b), 0);
    EXPECT_GT(strcmp(b, a), 0);

    strcpy(a, "a");
    strcpy(b, "aa");
    EXPECT_LT(strcmp(a, b), 0);
    EXPECT_GT(strcmp(b, a), 0);

    /* strcmp compares as-if unsigned */
    strcpy(a, "\x7f");
    strcpy(b, "\xff");
    EXPECT_LT(strcmp(a, b), 0);
    EXPECT_GT(strcmp(b, a), 0);
}

TEST_CASE(string_strncmp)
{
    char a[2];
    char b[2];

    a[0] = '\x7f'; a[1] = 'a';
    b[0] = '\xff'; b[1] = '\0';
    EXPECT_EQ(strncmp(a, b, 0), 0);
    /* difference before n; compares as-if unsigned */
    EXPECT_LT(strncmp(a, b, 1), 0);
    /* early termination of b */
    b[0] = a[0];
    EXPECT_GT(strncmp(a, b, 2), 0);
    /* early termination of a */
    a[1] = 0; b[1] = 'a';
    EXPECT_LT(strncmp(a, b, 2), 0);
}

TEST_CASE(string_strstr)
{
}

TEST_CASE(string_strchr)
{
}

TEST_CASE(string_strrchr)
{
}

TEST_CASE(string_strdup)
{
}

TEST_CASE(string_memcpy)
{
}

TEST_CASE(string_memccpy)
{
}

TEST_CASE(string_memchr)
{
    char buf[4] = { 'a', 0, 'c', 'd' };
    void *s = buf;
    void *p;

    /* length-limited */
    p = memchr(s, 'a', 0);
    EXPECT_EQ_P(p, NULL);
    p = memchr(s, 'a', 1);
    EXPECT_EQ(p - s, 0);
    /* is binary */
    p = memchr(s, 0, 4);
    EXPECT_EQ(p - s, 1);
    /* found case */
    p = memchr(s, 'c', 4);
    EXPECT_EQ(p - s, 2);
    /* not found case */
    p = memchr(s, 'x', 4);
    EXPECT_EQ_P(p, NULL);
}

TEST_CASE(string_memset)
{
}

TEST_CASE(string_memcmp)
{
}

TEST_CASE(string_memmove)
{
}

TEST_CASE(string_fmemset)
{
}

TEST_CASE(string_strerror)
{
}

TEST_CASE(string_strcasecmp)
{
}

TEST_CASE(string_strncasecmp)
{
}

TEST_CASE(string_strtok)
{
}

TEST_CASE(string_strpbrk)
{
}

TEST_CASE(string_strsep)
{
}

TEST_CASE(string_strcspn)
{
}

TEST_CASE(string_strspn)
{
}

TEST_CASE(string_strfry)
{
}
