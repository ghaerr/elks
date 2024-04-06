/* testsym - example program built with ELKS symbol table */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "debug/instrument.h"
#include "debug/syms.h"

void z()
{
    _print_regs();
    printf("Stack backtrace of z:\n");
    _print_stack(0xC0DE);
}

void y(int a)
{
    z();
}

void x()
{
    extern long lseek();

    //printf("x = %s\n", sym_text_symbol(x, -1));
    //printf("strlen+3 = %s\n", sym_text_symbol(strlen+3, 3));
    //printf("strlen = %s\n", sym_text_symbol(sym_fn_start_address(strlen+3), 3));
    //printf("lseek+20 = %s\n", sym_text_symbol(lseek+32, 32));
    //printf(".shift_loop = %s\n", sym_text_symbol((void *)0x110b, -1));
    //printf("errno = %s\n", sym_data_symbol(&errno, -1));
    y(1);
}

int main(int ac, char **av)
{
    x();
}
