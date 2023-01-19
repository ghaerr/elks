/*
 * ELKS instrumentation functions for ia16-elf-gcc
 *
 * June 2022 Greg Haerr
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "instrument.h"
#include "syms.h"

/* turn on for microcycle (CPU cycle/1000) timing info */
#define HAS_RDTSC       1   /* has RDTSC instruction: requires 386+ CPU */

static char ftrace;
static int count;
static unsigned int start_sp;
static unsigned int max_stack;

/* runs before main and rewrites argc/argv on stack if --ftrace found */
__attribute__((no_instrument_function,constructor(120)))
static void ftrace_checkargs(void)
{
    char **avp = __argv + 1;

    if ((*avp && !strcmp(*avp, "--ftrace")) || getenv("FTRACE")) {
        while (*avp) {
            *avp = *(avp + 1);
            avp++;
        }
        ftrace = 1;
        __argc--;
    }
    _get_micro_count();     /* init timer base */
}

/* every function this function calls must also be noinstrument!! */
void noinstrument __cyg_profile_func_enter_simple(void)
{
    if (!ftrace) return;

#if 1
    int **bp = __builtin_frame_address(0);  /* find BP on stack: BP, DI, SI, ret, arg1 */
    int *calling_fn = __builtin_return_address(0);  /* return address */
#else
    int **bp = (int **)&arg1 - 4;   /* find BP on stack: BP, DI, SI, ret, arg1 */
    int *calling_fn = bp[3];        /* return address */
    assert(bp == __builtin_frame_address(0));
    assert(calling_fn == __builtin_return_address(0));
#endif
    int i;
    char callsite[32];

    /* calc stack used */
    if (count == 0) start_sp = (unsigned int)bp;
    unsigned int stack_used = start_sp - (unsigned int)bp;
    if (stack_used > max_stack) max_stack = stack_used;

    /* calc caller address */
    i = _get_push_count(calling_fn);
    if (i & BP_PUSHED) {            /* caller pushed BP */
        bp = (int **)bp[0];         /* one level down to get caller BP */
        i &= COUNT_MASK;
    } else bp += 4;                 /* caller didn't push BP, skip past our ret addr */
    if (i >= 0)
        strcpy(callsite, sym_text_symbol(bp[i], 1));   /* return address of caller */
    else
        strcpy(callsite, "<unknown>");
    for (i=0; i<count; i++)
        putchar('|');
    printf(">%s, from %s, stack %u/%u", sym_text_symbol(calling_fn, 0), callsite,
        stack_used, max_stack);
#if HAS_RDTSC
    printf(" %lu ucycles", _get_micro_count());
#endif
    printf("\n");
    ++count;
}

void noinstrument __cyg_profile_func_exit_simple(void)
{
    if (ftrace)
        --count;
}

/* return CPU cycles / 1000 via RDTSC instruction */
unsigned long noinstrument _get_micro_count(void)
{
#if HAS_RDTSC
    static unsigned long long last_ts;

    unsigned long long ts = _get_rdtsc();    /* REQUIRES 386 CPU! */
    unsigned long diff = (ts - last_ts) / 1000;
    last_ts = ts;
    return diff;
#else
    return 0;
#endif
}

/***static char * noinstrument lltohexstr(unsigned long long val)
{
    static char buf[17];

    sprintf(buf,"%08lx%08lx", (long)(val >> 32), (long)val);
    return buf;
}***/
