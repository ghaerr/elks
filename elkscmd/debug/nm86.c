/* ELKS symbol table namelist */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "debug/syms.h"

static char * fstrncpy(char *dst, unsigned char __far *src, int n)
{
    do {
        *dst++ = *src++;
    } while (--n);
    *dst = '\0';
    return dst;
}

void nm(char *path)
{
    unsigned char __far *syms;
    unsigned char __far *p;
    static char name[64];

    syms = sym_read_exe_symbols(path);
    if (!syms) {
        if (errno)
            perror(path);
        else fprintf(stderr, "%s: no symbol table\n", path);
        return;
    }
    for (p = syms; p[TYPE] != '\0'; p = next(p)) {
        fstrncpy(name, p+SYMBOL, p[SYMLEN]);
        printf("0x%04x %c %s\n", *(unsigned short __far *)(p + ADDR), p[TYPE], name);
    }
}

int main(int ac, char **av)
{
    if (ac != 2)
        fprintf(stderr, "Usage: %s <a.out executable>\n", av[0]);
    else nm(av[1]);
}
