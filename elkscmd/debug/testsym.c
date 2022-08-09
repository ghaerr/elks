/* testsym - example program built with ELKS symbol table */
#include <stdio.h>
#include <string.h>
#include "syms.h"

void x()
{
    extern int errno;
    extern long lseek();

    printf("x = %s\n", sym_text_symbol(x, 1));
    printf("strlen = %s\n", sym_text_symbol(strlen+3, 1));
    printf("strlen = %s\n", sym_text_symbol(sym_fn_start_address(strlen+3), 1));
    printf("lseek = %s\n", sym_text_symbol(lseek+32, 1));
    printf(".shift_loop = %s\n", sym_text_symbol((void *)0x0C71, 1));
    printf("errno = %s\n", sym_data_symbol(&errno, 1));
}

int main(int ac, char **av)
{
    x();
}
