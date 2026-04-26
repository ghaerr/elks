/*
 * This helper is intended to be built with OpenWatcom as an OS/2 NE binary,
 * then converted to ELKS a.out with os2toelks.
 *
 * Goal: start life as a genuinely multisegment executable, then exec a normal
 * 2-segment ELKS binary. On a buggy kernel this can leave stale mm[] entries
 * behind in the replacement image.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __WATCOMC__
#pragma code_seg("MSCODE1")
static int code1(void) { return 11; }
#pragma code_seg("MSCODE2")
static int code2(void) { return 22; }
#pragma code_seg()

#pragma data_seg("MSDATA1")
static char data1[1024] = { 1 };
#pragma data_seg("MSDATA2")
static char data2[1024] = { 2 };
#pragma data_seg()
#else
static int code1(void) { return 11; }
static int code2(void) { return 22; }
static char data1[1024] = { 1 };
static char data2[1024] = { 2 };
#endif

static int touch_segments(void)
{
    data1[0]++;
    data2[0]++;
    return code1() + code2() + data1[0] + data2[0];
}

int main(int argc, char **argv)
{
    char *av[2];
    int v = touch_segments();

    if (argc < 2) {
        fprintf(stderr, "usage: %s /path/to/plain-helper\n", argv[0]);
        return 2;
    }

    /* Keep the segment-touch logic live. */
    if (v == -1)
        write(1, "impossible\n", 11);

    av[0] = argv[1];
    av[1] = (char *)0;
    execv(argv[1], av);
    perror("multiseg: execv(plain)");
    return 111;
}
