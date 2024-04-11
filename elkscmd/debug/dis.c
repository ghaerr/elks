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
#include <getopt.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linuxmt/minix.h>
#include "debug/syms.h"
#include "debug/instrument.h"
#include "disasm.h"
#if __ia16__
#include <linuxmt/mem.h>
#endif

#define KSYMTAB         "/lib/system.sym"

#define MAGIC       0x0301  /* magic number for executable progs */

char f_ksyms;
char f_syms;
unsigned short textseg, ftextseg, dataseg;

char * noinstrument getsymbol(int seg, int offset)
{
    static char buf[8];

    if (f_ksyms) {
        if (seg == textseg)
            return sym_text_symbol((void *)offset, -1);
        if (seg == ftextseg)
            return sym_ftext_symbol((void *)offset, -1);
        if (seg == dataseg)
            return sym_data_symbol((void *)offset, -1);
    }
    if (f_syms) {
        if (seg == dataseg)
            return sym_data_symbol((void *)offset, -1);
        return sym_text_symbol((void *)offset, -1);
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
    if (f_syms) {
        if (seg == dataseg)
            return ".data";
        return ".text";
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
        nextip = disasm(cs, ip, nextbyte_mem, dataseg);
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

    if (stat(filename, &sbuf) < 0 || (infp = fopen(filename, "r")) == NULL) {
        printf("Can't open %s\n", filename);
        return 1;
    }
    filesize = sbuf.st_size;
    f_syms = sym_read_exe_symbols(filename)? 1: 0;
    if (f_syms || (sym_hdr.type & 0xFFFF) == MAGIC) {
            fseek(infp, sym_hdr.hlen, SEEK_SET);
            filesize = sym_hdr.tseg;    /* FIXME no .fartext yet */
            textseg = 0;
            dataseg = 1;
    }

    while (ip < filesize) {
        if (!f_asmout) printf("%04hx  ", ip);
        nextip = disasm(textseg, ip, nextbyte_file, dataseg);
        ip = nextip;
    }
    fclose(infp);
    return 0;
}

void usage(void)
{
    printf("Usage: disasm [-k] [-a] [-s symfile] [[seg:off[#size] | filename]\n");
    exit(1);
}

int main(int ac, char **av)
{
    unsigned long seg = 0, off = 0;
    int fd, ch;
    char *symfile = NULL;
    long count = 22;

    while ((ch = getopt(ac, av, "kas:")) != -1) {
        switch (ch) {
        case 'a':
            f_asmout = 1;
            break;
        case 'k':
            f_ksyms = 1;
            symfile = KSYMTAB;
            break;
        case 's':
            f_syms = 1;
            symfile = optarg;
            break;
        default:
            usage();
        }
    }
    ac -= optind;
    av += optind;
    if (ac < 1)
        usage();

    if (symfile && !sym_read_symbols(symfile)) {
        printf("Can't open %s\n", symfile);
        exit(1);
    }

#if __ia16__
    if (f_ksyms) {
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
#endif

    if (strchr(*av, ':')) {
        sscanf(*av, "%lx:%lx#%ld", &seg, &off, &count);

        if (seg > 0xffff || off > 0xffff) {
            printf("Error: segment or offset larger than 0xffff\n");
            return 1;
        }
        disasm_mem((int)seg, (int)off, (int)count);
    } else return disasm_file(*av);
    return 0;
}
