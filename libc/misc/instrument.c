/* test -finstrument-functions-simple */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define noinstrument    __attribute__((no_instrument_function))

static size_t ftrace;
static size_t start_sp;
static size_t max_stack;
static int count;

/* runs before main and rewrites argc/argv on stack if --ftrace found */
__attribute__((no_instrument_function,constructor(120)))
static void ftrace_checkargs(void)
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
}

/* every function this function calls must also be noinstrument!! */
void noinstrument __cyg_profile_func_enter_simple(void)
{
    if (!ftrace) return;

    int **bp = __builtin_frame_address(0);  /* find BP on stack: BP, DI, SI, ret, arg1 */
    int *calling_fn = __builtin_return_address(0);  /* return address */
    int i;

    if (count == 0) start_sp = (size_t)bp;
    size_t stack_used = start_sp - (size_t)bp;
    if (stack_used > max_stack) max_stack = stack_used;

    for (i=0; i<count; i++)
        putchar('|');
    printf("@%04x stack %u/%u\n", (size_t)calling_fn, stack_used, max_stack);
    ++count;
}

void noinstrument __cyg_profile_func_exit_simple(void)
{
    if (ftrace)
        --count;
}
