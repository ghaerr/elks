#include "testlib.h"

#include <regex.h>
#include <stdlib.h>
#include <stdio.h>

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
