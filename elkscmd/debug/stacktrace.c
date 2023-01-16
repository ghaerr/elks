/*
 * ELKS stack trace functions for ia16-elf-gcc
 *
 * June 2022 Greg Haerr
 */
#include <stdio.h>
#include "stacktrace.h"
#include "syms.h"

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

/*
 * Return pushed word count and register bitmask by function at passed address,
 * used to traverse BP chain and display registers.
 */
int noinstrument calc_push_count(int *addr)
{
    int *fn = sym_fn_start_address(addr);   /* get fn start from address */
    char *fp = (char *)fn;
    int count = 0;

    int opcode = getcsbyte(fp++);
    if (opcode == 0x56)         /* push %si */
        count = (count+1) | SI_PUSHED, opcode = getcsbyte(fp++);
    if (opcode == 0x57)         /* push %di */
        count = (count+1) | DI_PUSHED, opcode = getcsbyte(fp++);
    if (opcode == 0x55          /* push %bp */
        || opcode == 0x59 && (int)fp < 0x40) /* temp kluge for crt0.S 'pop %cx' start */
        count = (count + 1) | BP_PUSHED, opcode = getcsbyte(fp);
    //printf("%s (%x) pushes %x\n", sym_text_symbol(addr, 1), (int)addr, count);
    return count;
}

/* display stack line and any subsequent lines w/o BP pushed */
static void noinstrument print_stack_line(int level, int **addr, int *fn, int flag)
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
    printf(" %s", sym_text_symbol(fn, 1));
    if (!(flag & BP_PUSHED))
        printf("*");
    printf(" (%04x)", (int)fn);
    printf("\n");
}

/* display call stack, arg1 ignored but displayed for testing */
void noinstrument print_stack(int arg1)
{
    int **bp = __builtin_frame_address(0);  /* address of saved BP in stack */
    int **addr = bp;
    int *fn = (int *)print_stack;
    int i = 0;

    printf("Level Addr    BP   DI   SI   Ret  Arg  Arg2 Arg3 Arg4\n"
           "~~~~~ ~~~~    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    do {
        int flag = calc_push_count(fn);
        int prev = flag;
        print_stack_line(i, addr, fn, flag);
        fn = addr[flag & COUNT_MASK];
        flag = calc_push_count(fn);
        if (flag & BP_PUSHED) {           /* caller pushed BP */
            addr = bp = (int **)bp[0];    /* one level down to get caller BP */
        } else {
            addr += (prev & COUNT_MASK) + 1;        /* skip past last ret addr */
        }
    } while (++i < 15 && fn != 0 && addr != 0);
}
