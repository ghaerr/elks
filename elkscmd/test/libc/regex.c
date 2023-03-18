#include "testlib.h"

#include <errno.h>
#include <fcntl.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void regerror(char *msg)
{
    printf("regerror: %s\n", msg);
}

TEST_CASE(regex_regcomp)
{
    struct regexp *r;

    r = regcomp("^literal$");
    ASSERT_NE_P(r, NULL);
    ASSERT_EQ(regexec(r, "literal"), 1);
    ASSERT_EQ(regexec(r, "not literal"), 0);
    ASSERT_EQ(regexec(r, "literally not"), 0);
    free(r);

    r = regcomp("a*");
    ASSERT_NE_P(r, NULL);
    ASSERT_EQ(regexec(r, ""), 1);
    ASSERT_EQ(regexec(r, "a"), 1);
    ASSERT_EQ(regexec(r, "ab"), 1);
    ASSERT_EQ(regexec(r, "baa"), 1);
    ASSERT_EQ(regexec(r, "b"), 1);

    r = regcomp("a+");
    ASSERT_NE_P(r, NULL);
    ASSERT_EQ(regexec(r, ""), 0);
    ASSERT_EQ(regexec(r, "a"), 1);
    ASSERT_EQ(regexec(r, "ab"), 1);
    ASSERT_EQ(regexec(r, "baa"), 1);
    ASSERT_EQ(regexec(r, "b"), 0);

    r = regcomp("(a|b)");
    ASSERT_NE_P(r, NULL);
    ASSERT_EQ(regexec(r, ""), 0);
    ASSERT_EQ(regexec(r, "a"), 1);
    ASSERT_EQ(regexec(r, "b"), 1);
    ASSERT_EQ(regexec(r, "c"), 0);

    r = regcomp("ab?c");
    ASSERT_NE_P(r, NULL);
    ASSERT_EQ(regexec(r, ""), 0);
    ASSERT_EQ(regexec(r, "a"), 0);
    ASSERT_EQ(regexec(r, "ac"), 1);
    ASSERT_EQ(regexec(r, "abc"), 1);
    ASSERT_EQ(regexec(r, "abbc"), 0);

    r = regcomp("a.c");
    ASSERT_NE_P(r, NULL);
    ASSERT_EQ(regexec(r, ""), 0);
    ASSERT_EQ(regexec(r, "a"), 0);
    ASSERT_EQ(regexec(r, "ac"), 0);
    ASSERT_EQ(regexec(r, "abc"), 1);
    ASSERT_EQ(regexec(r, "axc"), 1);

    r = regcomp("[b-dB-D]");
    ASSERT_NE_P(r, NULL);
    ASSERT_EQ(regexec(r, ""), 0);
    ASSERT_EQ(regexec(r, "a"), 0);
    ASSERT_EQ(regexec(r, "b"), 1);
    ASSERT_EQ(regexec(r, "d"), 1);
    ASSERT_EQ(regexec(r, "B"), 1);
    ASSERT_EQ(regexec(r, "D"), 1);
    ASSERT_EQ(regexec(r, "e"), 0);
    ASSERT_EQ(regexec(r, "E"), 0);

    r = regcomp("^[ \t]*[a-zA-Z_][a-zA-Z_0-9]*[ \t]*=[ \t]*[\"']?.*[\"']?;?$");
    ASSERT_NE_P(r, NULL);
    ASSERT_EQ(regexec(r, "  python = False"), 1);
    ASSERT_EQ(regexec(r, "i = 42;"), 1);
    ASSERT_EQ(regexec(r, "key = \"value\";"), 1);
    free(r);
}

TEST_CASE(regex_expandwildcards)
{
    const char *root = "/tmp/libcwild";
    const char *file1 = "abc123.txt";
    const char *file2 = "esc[2].txt";
    const char *file3 = "esc\\*.txt";
    char *file;
    char path[80];
    char *argv[3];

    mkdir(root, 0755);
    strcpy(path, root);
    file = path + strlen(root);
    *file++ = '/';
    strcpy(file, file1);
    creat(path, 0644);
    strcpy(file, file2);
    creat(path, 0644);
    strcpy(file, file3);
    creat(path, 0644);

    struct {
        const char *pattern;
        int n;
        int r;
    } tests[] = {
        { "abc123.txt",     3, 0 },
        { "abc123.tx?",     3, 1 },
        { "abc123.txt?",    3, -ENOENT },
        { "abc123.txt*",    3, 1 },
        /* corner-case bug, maybe not worth fixing:
         * pattern is not a wildcard so should return 0
         */
        { "esc\\[2\\].txt", 3, 1 },
        { "abc*[34].txt",   3, 1 },
        { "abc12[45].txt",  3, -ENOENT },
        { "[]",             3, -ENOENT },
        { "*",              0, -ENOBUFS },
        { "*",              3, 3 },
        { "*\\",            3, -ENOENT },
        { "*.*",            3, 3 },
        { "*.txt",          3, 3 },
        { "*.txt",          1, -ENOBUFS },
        { "*.bin",          3, -ENOENT },
        { "??????????",     3, 2 },
        { "[ax]*",          3, 1 },
        { "*c[1\\[]*.???",  3, 2 },
        { "esc\\[*.txt",    3, 1 },
        { "esc\\[?\\].txt", 3, 1 },
        { "esc*.txt",       3, 2 },
        { "esc\\\\\\*.tx?", 3, 1 },
        { "esc[\\\\\[]*",   3, 2 },
        { NULL,             0, 0 }
    };
    for (int i = 0; tests[i].pattern; ++i) {
        CAPTURE("pattern", tests[i].pattern);
        strcpy(file, tests[i].pattern);
        int r = expandwildcards(path, tests[i].n, argv);
        EXPECT_EQ(r, tests[i].r);
        freewildcards();
    }

    strcpy(file, file1);
    unlink(path);
    strcpy(file, file2);
    unlink(path);
    strcpy(file, file3);
    unlink(path);
    rmdir(root);
}
