/*
 * ELKS stack trace functions for ia16-elf-gcc
 *
 * June 2022 Greg Haerr
 */
#include <stdio.h>
#include <unistd.h>
#include "debug/instrument.h"
#include "debug/syms.h"

#define STACKCOLS   8   /* # of stack address columns */

/* ia-16 C function stack layout (hi to low):
+--------------+
| arg1         | bp[4]
+--------------+
| ret addr     | bp[3]
+--------------+
| si           | bp[2] (optional, always 1st push)
+--------------+
| di           | bp[1] (optional)
+--------------+
| bp           | bp[0], <- BP (optional, always last push)
+--------------+
| temp vars    | bp[-1], <- SP
+--------------+
*/

/* display stack line and any subsequent lines w/o BP pushed */
static void noinstrument _print_stack_line(int level, int **addr, int *fn,
    int *fnstart, int flag)
{
    int j = 0;

    printf("%4d: %04x =>", level, (int)addr);
    do {
        if ((j == 0 && !(flag & BP_PUSHED))
         || (j == 1 && !(flag & DI_PUSHED))
         || (j == 2 && !(flag & SI_PUSHED)))
            printf("     ");
        else printf(" %04x", (int)*addr++);
    } while (++j < STACKCOLS);
    printf(" (%04x)", (int)fn);
    printf(" %s", sym_text_symbol(fn, (size_t)fn - (size_t)fnstart));
    if (!(flag & BP_PUSHED))
        printf("*");
    printf("\n");
}

/* display call stack, arg1 ignored but displayed for testing */
void noinstrument _print_stack(int arg1)
{
    int **bp = __builtin_frame_address(0);  /* address of saved BP in stack */
    int **addr = bp;
    int *fn = (int *)_print_stack;
    int i = 0;

    sym_read_exe_symbols(__program_filename);
    printf("Level Addr    BP   DI   SI   Ret  Arg  Arg2 Arg3 Arg4\n"
           "~~~~~ ~~~~    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    do {
        int *fnstart = _get_fn_start_address(fn);
        int flag = _get_push_count(fnstart);
        int prev = flag;
        _print_stack_line(i, addr, fn, fnstart, flag);
        fn = addr[flag & COUNT_MASK];
        fnstart = _get_fn_start_address(fn);
        flag = _get_push_count(fnstart);
        if (flag & BP_PUSHED) {           /* caller pushed BP */
            addr = bp = (int **)bp[0];    /* one level down to get caller BP */
        } else {
            addr += (prev & COUNT_MASK) + 1;        /* skip past last ret addr */
        }
    } while (++i < 15 && fn != 0 && addr != 0);
}
