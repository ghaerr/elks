/* testsym - example program built with ELKS symbol table */
#include <stdio.h>
#include <string.h>
#include "syms.h"
#include "stacktrace.h"
#include "disasm.h"

char * noinstrument getsymbol(int seg, int offset)
{
    static char buf[32];

    if (seg == getcs())
        return sym_text_symbol((void *)offset, 1);
    sprintf(buf, "%04x", offset);
    return buf;
}

#define peekb(cs,ip)  (*(unsigned char __far *)(((unsigned long)(cs) << 16) | (int)(ip)))

void disassemble(int cs, void *ip, int opcount)
{
    int n;
    void *nextip;

    if (!opcount) opcount = 32767;
    printf("Disassembly of %s:\n", getsymbol(cs, (int)ip));
    for (n=0; n<opcount; n++) {
        nextip = disasm(cs, ip);
        if (peekb(cs, ip) == 0xc3)  /* RET */
            break;
        ip = nextip;
    }
}

void z()
{
    printf("Stack backtrace of z:\n");
    print_stack(0xDEAD);
    disassemble(getcs(), z, 0);
    disassemble(getcs(), disassemble, 0);
}

void y(int a)
{
    z();
}

void x()
{
    extern int errno;
    extern long lseek();

    printf("x = %s\n", sym_text_symbol(x, 1));
    printf("strlen+3 = %s\n", sym_text_symbol(strlen+3, 1));
    printf("strlen = %s\n", sym_text_symbol(sym_fn_start_address(strlen+3), 1));
    printf("lseek+20 = %s\n", sym_text_symbol(lseek+32, 1));
    //printf(".shift_loop = %s\n", sym_text_symbol((void *)0x0C71, 1));
    printf("errno = %s\n", sym_data_symbol(&errno, 1));
    y(1);
}

int main(int ac, char **av)
{
    x();
}
