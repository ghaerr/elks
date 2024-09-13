/*
 * ELKS instrumentation functions for ia16-elf-gcc
 *
 * June 2022 Greg Haerr
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/rtinit.h>
#include "debug/instrument.h"
#include "debug/syms.h"

/* turn on for microcycle (CPU cycle/1000) timing info */
#define HAS_RDTSC       0   /* has RDTSC instruction: requires 386+ CPU */

static size_t ftrace;
static size_t start_sp;
static size_t max_stack;
static int count;

/* runs before main and rewrites argc/argv on stack if --ftrace found */
#pragma GCC diagnostic ignored "-Wprio-ctor-dtor"
static noinstrument CONSTRUCTOR(ftrace_checkargs, _INIT_PRI_FTRACE);
static noinstrument void ftrace_checkargs(void)
{
    char **avp = __argv + 1;

    ftrace = (size_t)getenv("FTRACE");
    if ((*avp && !strcmp(*avp, "--ftrace"))) {
        while (*avp) {
            *avp = *(avp + 1);
            avp++;
        }
        __argc--;
        ftrace = 1;
    }
    if (ftrace)
        sym_read_exe_symbols(__program_filename);
#if HAS_RDTSC
    _get_micro_count();     /* init timer base */
#endif
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
    int i, offset;
    char callsite[32];

    /* calc stack used */
    if (count == 0) start_sp = (size_t)bp;
    size_t stack_used = start_sp - (size_t)bp;
    if (stack_used > max_stack) max_stack = stack_used;

    /* calc caller address */
    i = _get_push_count(_get_fn_start_address(calling_fn));
    if (i & BP_PUSHED) {            /* caller pushed BP */
        bp = (int **)bp[0];         /* one level up to get caller BP */
        i &= COUNT_MASK;
                                    /* return address of caller */
        offset = (size_t)bp[i] - (size_t)_get_fn_start_address(bp[i]);
        strcpy(callsite, sym_text_symbol(bp[i], offset));
    } else {                        /* caller didn't push BP */
        strcpy(callsite, "<unknown>");
    }
    for (i=0; i<count; i++)
        fprintf(stderr, "|");
    fprintf(stderr, ">%s, from %s, stack %u/%u", sym_text_symbol(calling_fn, 0), callsite,
        stack_used, max_stack);
#if HAS_RDTSC
    fprintf(stderr, " %lu ucycles", _get_micro_count());
#endif
    fprintf(stderr, "\n");
    ++count;
}

void noinstrument __cyg_profile_func_exit_simple(void)
{
    if (ftrace)
        --count;
}

#if HAS_RDTSC
/* return CPU cycles / 1000 via RDTSC instruction */
unsigned long noinstrument _get_micro_count(void)
{
    static unsigned long long last_ts;

    unsigned long long ts = _get_rdtsc();    /* REQUIRES 386 CPU! */
    unsigned long diff = (ts - last_ts) / 1000;
    last_ts = ts;
    return diff;
}
#endif

/***static char * noinstrument lltohexstr(unsigned long long val)
{
    static char buf[17];

    sprintf(buf,"%08lx%08lx", (long)(val >> 32), (long)val);
    return buf;
}***/
