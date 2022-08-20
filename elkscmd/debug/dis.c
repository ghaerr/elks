/*
 * ELKS 8086 Disassembler
 *
 * Aug 2022 Greg Haerr
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linuxmt/mem.h>
#include "syms.h"
#include "disasm.h"

#define KSYMTAB         "/lib/system.sym"
char f_ksyms;
unsigned short textseg, ftextseg, dataseg;

char * noinstrument getsymbol(int seg, int offset)
{
    static char buf[8];

    if (f_ksyms) {
        if (seg == textseg)
            return sym_text_symbol((void *)offset, 1);
        if (seg == ftextseg)
            return sym_ftext_symbol((void *)offset, 1);
        if (seg == dataseg)
            return sym_data_symbol((void *)offset, 1);
    }
    sprintf(buf, f_asmout? "0x%04x": "%04x", offset);
    return buf;
}

char * noinstrument getsegsymbol(int seg)
{
    static char buf[8];

    if (f_ksyms) {
        if (seg == textseg)
            return ".text";
        if (seg == ftextseg)
            return ".fartext";
        if (seg == dataseg)
            return ".data";
    }
    sprintf(buf, f_asmout? "0x%04x": "%04x", seg);
    return buf;
}

#define peekb(cs,ip)  (*(unsigned char __far *)(((unsigned long)(cs) << 16) | (int)(ip)))

static int nextbyte_mem(int cs, int ip)
{
    int b = peekb(cs, ip);
    if (!f_asmout) printf("%02x ", b);
    else f_outcol = 0;
    return b;
}

void disasm_mem(int cs, int ip, int opcount)
{
    int n;
    int nextip;

    if (!opcount) opcount = 32767;
    if (!f_asmout) printf("Disassembly of %s:\n", getsymbol(cs, (int)ip));
    for (n=0; n<opcount; n++) {
        if (!f_asmout) printf("%04hx:%04hx  ", cs, ip);
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
    if (!f_asmout) printf("%02x ", b);
    else f_outcol = 0;
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
        if (!f_asmout) printf("%04hx  ", ip);
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

    while (ac > 1 && av[1][0] == '-') {
        if (!strcmp(av[1], "-a")) {
            f_asmout = 1;
            ac--;
            av++;
        }
        if (!strcmp(av[1], "-k")) {
            f_ksyms = 1;
            ac--;
            av++;
        }
    }
    if (ac != 2) {
        printf("Usage: disasm [-k] [-a] [[seg:off[#size] | filename]\n");
        return 1;
    }
    if (f_ksyms) {
        int fd;
        if (!sym_read_symbols(KSYMTAB)) {
            printf("Can't open %s\n", KSYMTAB);
            exit(1);
        }
        fd = open("/dev/kmem", O_RDONLY);
        if (fd < 0
            || ioctl(fd, MEM_GETCS, &textseg) < 0
            || ioctl(fd, MEM_GETDS, &dataseg) < 0
            || ioctl(fd, MEM_GETFARTEXT, &ftextseg) < 0) {
                printf("Can't get kernel segment values\n");
                exit(1);
        }
        close(fd);
    }

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
