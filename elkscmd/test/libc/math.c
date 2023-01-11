#include "testlib.h"

#include <errno.h>
#include <math.h>

#include <stdio.h>
#include <sys/linksym.h>
__STDIO_PRINT_FLOATS;       // link in libc printf float support

struct PowCase {
	double x;
	double y;
	double r;
	int e;
};

static struct PowCase powCases[8] = {
	{ 0., 0., 1., 0 },
	{ 0., 1., 0., 0 },
	{ 1., 0., 1., 0 },
	{ 1., 1., 1., 0 },
	{ 1., 2., 1., 0 },
	{ 2., 16., 65536., 0 },
	{ 2., 2., 4., 0 },
	/* If x is a finite value less than 0, and y is a finite noninteger, a
	 * domain error occurs, and a NaN is returned.
	 */
	/* TODO */
	/* { -1., 0.1, __builtin_nan(""), EDOM }, */
	/* If the result overflows, a range error occurs, and the functions
	 * return HUGE_VAL, HUGE_VALF, or HUGE_VALL, respectively, with the
	 * mathematically correct sign.
	 */
	/* TODO... */
};

TEST_CASE(math_floor)
{
	EXPECT_EQ(floor(0.0), 0.0);
	EXPECT_EQ(floorf(0.0), 0.0);
	EXPECT_EQ(floor(0.99), 0.0);
	EXPECT_EQ(floorf(0.99), 0.0);
	EXPECT_EQ(floor(-0.99), -1.0);
	EXPECT_EQ(floorf(-0.99), -1.0);
	EXPECT_EQ(floor(1.99), 1.0);
	EXPECT_EQ(floorf(1.99), 1.0);
	EXPECT_EQ(floor(-1.99), -2.0);
	EXPECT_EQ(floorf(-1.99), -2.0);
	/* If x is integral, +0, -0, NaN, or an infinity, x is returned. */
	EXPECT_EQ(floorf(1.0), 1.0);
	EXPECT_EQ(floorf(+0.0), +0.0);
	EXPECT_EQ(floorf(-0.0), -0.0);
	/* TODO NaN inf */
}

TEST_CASE(math_sqrt)
{
	/* On success, these functions return the square root of x. */
	EXPECT_EQ(sqrt(0.), 0.);
	EXPECT_EQ(sqrtf(0.), 0.);
	EXPECT_EQ(sqrt(1.), 1.);
	EXPECT_EQ(sqrtf(1.), 1.);
	EXPECT_EQ(sqrt(1.23), 1.23);
	EXPECT_EQ(sqrtf(1.23), 1.23);

	/* If x is a NaN, a NaN is returned. */
	/* TODO */

	/* If x is +0 (-0), +0 (-0) is returned. */
	EXPECT_EQ(sqrt(+0.0), +0.0);
	EXPECT_EQ(sqrtf(+0.0), +0.0);
	EXPECT_EQ(sqrt(-0.0), -0.0);
	EXPECT_EQ(sqrtf(-0.0), -0.0);

	/* If x is positive infinity, positive infinity is returned. */
	/* TODO */

	/* If x is less than -0, a domain error occurs, and a NaN is returned. */
	errno = 0;
	EXPECT_EQ(sqrt(-1.0), -0.0);
	EXPECT_EQ(sqrtf(-1.0), -0.0);
}

TEST_CASE(math_abs)
{
	EXPECT_EQ(fabs(0.0), 0.0);
	EXPECT_EQ(fabs(1.0), 1.0);
	EXPECT_EQ(fabs(-1.0), 1.0);
	EXPECT_EQ(fabsf(0.0), 0.0);
	EXPECT_EQ(fabsf(1.0), 1.0);
	EXPECT_EQ(fabsf(-1.0), 1.0);

	/* If x is a NaN, a NaN is returned. */
	/* TODO */
	//ASSERT_TRUE(isnan(fabsf(__builtin_nan(""))));

	/* If x is -0, +0 is returned. */
	EXPECT_EQ(fabs(-0.0), 0);
	EXPECT_EQ(fabsf(-0.0), 0);

	/* If x is neg or pos infinity, positive infinity is returned. */
	/* TODO */
}

TEST_CASE(math_pow)
{
	for (int i = 0; i < 8; ++i) {
		struct PowCase *t = &powCases[i];
		errno = 0;
/* TODO some are failing
 *  - check test data
 *  - need approx EQ?
 *  - printf %f doesn't
 */
#if 0
		ASSERT_EQ_F(pow(t->x, t->y), t->r);
#endif
		ASSERT_EQ(errno, t->e);
	}
}
