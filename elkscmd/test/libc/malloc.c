#include "testlib.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

TEST_CASE(malloc_malloc_free) {
    void *p;

    /* malloc(0) may return NULL or a pointer which can be passed to free */
    errno = 0;
    p = malloc(0);
    EXPECT_EQ(errno, 0);
    if (p != NULL) {
        free(p);
    }

    /* TODO:BUG: causes hang at 100% CPU */
#if 0
    errno = 0;
    p = malloc((size_t) - 1);
    if (p == NULL) {
        EXPECT_EQ(errno, ENOMEM);
    } else {
        EXPECT_EQ(errno, 0);
        memset(p, 0xff, (size_t) - 1);
        free(p);
    }
#endif

    /* strange sizes are fine; memory is writable and free-able */
    for (int i = 1; i < 1024; i += 123) {
        errno = 0;
        p = malloc(i);
        EXPECT_EQ(errno, 0);
        ASSERT_NE_P(p, NULL);
        memset(p, 0xff, i);

        /* free must not change errno */
        errno = 123;
        free(p);
        EXPECT_EQ(errno, 123);
    }
}

TEST_CASE(malloc_calloc) {
    void *p;

    errno = 0;
    p = calloc(0, 1);
    EXPECT_EQ(errno, 0);
    if (p != NULL) {
        free(p);
    }

    p = calloc(((unsigned)-1)>>2, 5);
    EXPECT_EQ(errno, ENOMEM);
    EXPECT_EQ_P(p, NULL);
}

TEST_CASE(malloc_realloc) {
    char *p;

    p = realloc(NULL, 32);
    ASSERT_NE_P(p, NULL);
    memset(p, 'A', 32);

    /* shrink */
    p = realloc(p, 1);
    ASSERT_NE_P(p, NULL);
    memset(p, 'B', 1);

    /* grow */
    p = realloc(p, 64);
    ASSERT_NE_P(p, NULL);
    EXPECT_EQ(p[0], 'B');
    memset(p, 'C', 64);

    free(p);
}

TEST_CASE(malloc_alloca) {
    void *p;

    p = alloca(1);
    EXPECT_NE_P(p, NULL);

    /* TODO nested function calls */
    /* TODO vs setjmp / longjmp */
}
