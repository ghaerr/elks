/* ELKS symbol table namelist */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "syms.h"

void nm(char *path)
{
    unsigned char *syms = sym_read_symbols(path);
    unsigned char *p;

    if (!syms) {
        if (errno)
            perror(path);
        else fprintf(stderr, "%s: no symbol table\n", path);
        return;
    }
    for (p = syms; p[TYPE] != '\0'; p = next(p)) {
        printf("0x%.4x %c %.*s\n", *(unsigned short *)(p + ADDR), p[TYPE],
            p[SYMLEN], p+SYMBOL);
    }
}

int main(int ac, char **av)
{
    if (ac != 2)
        fprintf(stderr, "Usage: nm86 <a.out executable>\n");
    else nm(av[1]);
}
