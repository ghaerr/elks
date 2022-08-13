/*
 * ELKS 8086 Disassembler
 *
 * Aug 2022 Greg Haerr
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
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

static int nextbyte_mem(int cs, int ip)
{
    int b = peekb(cs, ip);
    printf("%02x ", b);
    return b;
}

void disasm_mem(int cs, int ip, int opcount)
{
    int n;
    int nextip;

    if (!opcount) opcount = 32767;
    printf("Disassembly of %s:\n", getsymbol(cs, (int)ip));
    for (n=0; n<opcount; n++) {
        printf("%04hx:%04hx  ", cs, ip);
        nextip = disasm(cs, ip, nextbyte_mem);
        if (opcount == 32767 && peekb(cs, ip) == 0xc3)  /* RET */
            break;
        ip = nextip;
    }
}

static FILE *infp;

static int nextbyte_file(int cs, int ip)
{
    int b = fgetc(infp);
    printf("%02x ", b);
    return b;
}

int disasm_file(char *filename)
{
    int ip = 0;
    int nextip;
    long filesize;
    struct stat sbuf;

    if ((infp = fopen(filename, "r")) == NULL) {
        printf("Can't open %s\n", filename);
        return 1;
    }
    if (stat(filename, &sbuf) < 0)
        filesize = 0;
    else filesize = sbuf.st_size;

    while (ip < filesize) {
        printf("%04hx  ", ip);
        nextip = disasm(0, ip, nextbyte_file);
        ip = nextip;
    }
    fclose(infp);
    return 0;
}

int main(int ac, char **av)
{
    unsigned long seg = 0, off = 0;
    long count = 22;

    if (ac != 2) {
        printf("Usage: disasm [[seg:off[#size] | filename]\n");
        return 1;
    }
    printf("CS = %x\n", getcs());

    if (strchr(av[1], ':')) {
        sscanf(av[1], "%lx:%lx#%ld", &seg, &off, &count);

        if (seg > 0xffff || off > 0xffff) {
            printf("Error: segment or offset larger than 0xffff\n");
            return 1;
        }
        disasm_mem((int)seg, (int)off, (int)count);
    } else return disasm_file(av[1]);
    return 0;
}
