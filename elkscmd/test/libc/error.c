#include "test.h"

#include <string.h>

#define N_ERR_TESTS 5
struct err_tests {
	int errno;
	const char *msg;
} err_map[N_ERR_TESTS] = {
	{  -1, "Unknown error -1" },
	{   0, "Unknown error 0" },
	{   1, "Operation not permitted " },
	{ 129, "Server error " },
	{ 999, "Unknown error 999" },
};

/* FIXME /etc/perror is generated with trailing spaces */
void test_strerror()
{
	int i;
	for (i = 0; i < N_ERR_TESTS; ++i) {
		const char *msg = strerror(err_map[i].errno);
		if (strcmp(msg, err_map[i].msg)) {
			printf("strerror: incorrect message: errno=%d\n"
				"\tactual   '%s'\n"
				"\texpected '%s'\n",
				err_map[i].errno, msg, err_map[i].msg);
			fail++;
		}
	}
}

