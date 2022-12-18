#include "testlib.h"

#include <arpa/inet.h>

struct Test {
	unsigned char a, b, c, d;
	const char *ip;
};

static struct Test tests[] = {
	{ 1, 2, 3, 4, "1.2.3.4" },
	{ 252, 253, 254, 255, "252.253.254.255" },
};

TEST_CASE(inet_aton_ntoa)
{
	for (int i = 0; i < 2; ++i) {
		struct Test *t = &tests[i];
		ipaddr_t a = in_aton(t->ip);
		unsigned char *p = (unsigned char*)&a;
		EXPECT_EQ(p[0], t->a);
		EXPECT_EQ(p[1], t->b);
		EXPECT_EQ(p[2], t->c);
		EXPECT_EQ(p[3], t->d);

		EXPECT_STREQ(in_ntoa(a), t->ip);
	}
}

TEST_CASE(inet_gethostbyname)
{
	ipaddr_t a;
	a = in_gethostbyname(tests[0].ip);
	EXPECT_STREQ(in_ntoa(a), tests[0].ip);

	a = in_gethostbyname("localhost");
	EXPECT_STREQ(in_ntoa(a), "127.0.0.1");

	/* TODO sandbox /etc/hosts? */
}

TEST_CASE(inet_resolv)
{
	/* TODO */
}
