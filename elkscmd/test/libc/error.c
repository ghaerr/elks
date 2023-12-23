#include "testlib.h"

#include <stdio.h>
#include <string.h>

#define N_ERR_TESTS 4
struct err_tests {
    int errno;
    const char *msg;
} err_map[N_ERR_TESTS] = {
    {   0, "Error 0" },
    {   1, "Operation not permitted " },
    { 129, "Server error " },
    { 999, "Error 999" },
};

/* TODO:BUG: /etc/perror is generated with trailing spaces */
TEST_CASE(error_strerror)
{
    for (int i = 0; i < N_ERR_TESTS; ++i) {
        const char *msg = strerror(err_map[i].errno);
        EXPECT_STREQ(msg, err_map[i].msg);
    }
}

