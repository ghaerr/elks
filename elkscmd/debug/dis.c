/*
 * ELKS 8086 Disassembler
 *
 * Aug 2022 Greg Haerr
 */
#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>
//#include <unistd.h>
#include "syms.h"
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

int main(int ac, char **av)
{
    unsigned long seg = 0, off = 0;
    long count = 22;

    if (ac != 2) {
        printf("Usage: disasm seg:off[#size]\n");
        return 1;
    }
    printf("CS = %x\n", getcs());
    sscanf(av[1], "%lx:%lx#%ld", &seg, &off, &count);

    if (seg > 0xffff || off > 0xffff) {
        printf("Error: segment or offset larger than 0xffff\n");
        return 1;
    }
    disassemble((int)seg, (void *)(int)off, (int)count);
    return 0;
}
