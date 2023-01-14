/* test -finstrument-functions-simple */
#include <stdio.h>
#define noinstrument    __attribute__((no_instrument_function))

static int count;
static int **start_sp;
static unsigned int max_stack;

/* every function this function calls must also be noinstrument!! */
void noinstrument __cyg_profile_func_enter_simple(void)
{
    int **bp = __builtin_frame_address(0);  /* find BP on stack: BP, DI, SI, ret, arg1 */
    int *calling_fn = __builtin_return_address(0);  /* return address */
    int i;

    /* calc stack used */
    if (count == 0) start_sp = bp;
    unsigned int stack_used = start_sp - bp;
    if (stack_used > max_stack) max_stack = stack_used;

    for (i=0; i<count; i++)
        putchar('|');
    printf(">%x, stack %u/%u\n", (int)calling_fn, stack_used, max_stack);
    ++count;
}

void noinstrument __cyg_profile_func_exit_simple(void)
{
    --count;
}
