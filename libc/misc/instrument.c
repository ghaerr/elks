/* test -finstrument-functions-simple */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define noinstrument    __attribute__((no_instrument_function))

static char ftrace;
static int count;
static unsigned int start_sp;
static unsigned int max_stack;

/* runs before main and rewrites argc/argv on stack if --ftrace found */
__attribute__((no_instrument_function,constructor(120)))
static void checkargs(void)
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
}

/* every function this function calls must also be noinstrument!! */
void noinstrument __cyg_profile_func_enter_simple(void)
{
    if (!ftrace) return;

    int **bp = __builtin_frame_address(0);  /* find BP on stack: BP, DI, SI, ret, arg1 */
    int *calling_fn = __builtin_return_address(0);  /* return address */
    int i;

    if (count == 0) start_sp = (unsigned int)bp;
    unsigned int stack_used = start_sp - (unsigned int)bp;
    if (stack_used > max_stack) max_stack = stack_used;

    for (i=0; i<count; i++)
        putchar('|');
    printf("@%04x stack %u/%u\n", (int)calling_fn, stack_used, max_stack);
    ++count;
}

void noinstrument __cyg_profile_func_exit_simple(void)
{
    if (ftrace)
        --count;
}
