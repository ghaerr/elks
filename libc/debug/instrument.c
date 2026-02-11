/*
 * ELKS instrumentation functions for ia16-elf-gcc
 *
 * June 2022 Greg Haerr
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <paths.h>
#include <sys/rtinit.h>
#include <linuxmt/prectimer.h>
#include "debug/instrument.h"
#include "debug/syms.h"

#define getsp() __extension__ ({        \
        unsigned int _v;                \
        asm volatile ("mov %%sp,%%ax"   \
            :"=a" (_v)                  \
            : /* no input */            \
        );                              \
        _v; })

static size_t ftrace;
static size_t start_sp;
static size_t max_stack;
static int count;
static int *save_calling_fn[64];

/* runs before main and rewrites argc/argv on stack if --ftrace found */
#pragma GCC diagnostic ignored "-Wprio-ctor-dtor"
static noinstrument CONSTRUCTOR(ftrace_checkargs, _INIT_PRI_FTRACE);
static noinstrument void ftrace_checkargs(void)
{
    char **avp = __argv + 1;
    char *p;
    int fd;

    if (*avp && !strncmp("--ftrace", *avp, 8)) {
        p = *avp + 8;
        while (*avp) {
            *avp = *(avp + 1);
            avp++;
        }
        __argc--;
    } else
        p = getenv("FTRACE");
    if (p) {
        if (*p) ftrace = *p - '0';
        else ftrace = 1;
    }
    if (ftrace) {
        fd = open(_PATH_CONSOLE, O_WRONLY);
        if (fd >= 0) {
            dup2(fd, 2);
            close(fd);
        }
        sym_read_exe_symbols(__program_filename);
        if (!init_ptime())          /* init precision time routine */
            _exit(1);
        get_ptime();
    }
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
    size_t stack_used;
    static char callsite[32];       /* stack usage can be very deep */

    /* calc stack used */
    if (count == 0) start_sp = (size_t)bp;
    stack_used = start_sp - (size_t)bp;
    if ((int)stack_used < 0)        /* handle setjmp */
        start_sp = (size_t)bp;
    else if (stack_used > max_stack)
        max_stack = stack_used;

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
    stderr[0].mode = (stderr[0].mode & ~__MODE_BUF) | _IOLBF;
    fprintf(stderr, "(%d)", getpid());
    for (i=0; i<count; i++)
       fputc('|', stderr);
    fprintf(stderr, ">%s, from %s %d/%u SP %x %lk", sym_text_symbol(calling_fn, 0),
        callsite, stack_used, max_stack, getsp(), get_ptime());
    fputc('\n', stderr);
    save_calling_fn[count] = calling_fn;
    if (ftrace & 2) _print_stack(0);
    ++count;
}

void noinstrument __cyg_profile_func_exit_simple(void)
{
    if (ftrace) {
        int *calling_fn = save_calling_fn[--count];
        fprintf(stderr, "(%d)", getpid());
        for (int i=0; i<count; i++)
            fputc('|', stderr);
        fprintf(stderr, "<%s EXIT SP %x\n", sym_text_symbol(calling_fn, 0), getsp());
    }
}

#if UNUSED
/* return CPU cycles / 1000 via RDTSC instruction */
unsigned long noinstrument _get_micro_count(void)
{
    static unsigned long long last_ts;

    unsigned long long ts = _get_rdtsc();    /* REQUIRES 386 CPU! */
    unsigned long diff = (ts - last_ts) >> 10;
    last_ts = ts;
    return diff;
}
#endif
