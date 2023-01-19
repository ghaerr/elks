/*
 * ELKS shared instrumentation functions for ia16-elf-gcc
 *
 * June 2022 Greg Haerr
 */
#include <stdio.h>
#include "instrument.h"
#include "syms.h"

/*
 * Return pushed word count and register bitmask by function at passed address,
 * used to traverse BP chain and display registers.
 */
int noinstrument _get_push_count(int *addr)
{
    int *fn = sym_fn_start_address(addr);   /* get fn start from address */
    char *fp = (char *)fn;
    int count = 0;

    int opcode = _get_csbyte(fp++);
    if (opcode == 0x56)         /* push %si */
        count = (count+1) | SI_PUSHED, opcode = _get_csbyte(fp++);
    if (opcode == 0x57)         /* push %di */
        count = (count+1) | DI_PUSHED, opcode = _get_csbyte(fp++);
    if (opcode == 0x55          /* push %bp */
        || opcode == 0x59 && (int)fp < 0x40) /* temp kluge for crt0.S 'pop %cx' start */
        count = (count + 1) | BP_PUSHED, opcode = _get_csbyte(fp);
    //printf("%s (%x) pushes %x\n", sym_text_symbol(addr, 1), (int)addr, count);
    return count;
}
