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

void disasm_mem(int cs, void *ip, int opcount)
{
    int n;
    void *nextip;

    if (!opcount) opcount = 32767;
    printf("Disassembly of %s:\n", getsymbol(cs, (int)ip));
    for (n=0; n<opcount; n++) {
        nextip = disasm(cs, ip);
        if (opcount == 32767 && peekb(cs, ip) == 0xc3)  /* RET */
            break;
        ip = nextip;
    }
}

void disasm_buf(char *addr, int size)
{
    char *ip = addr;
    void *nextip;

    while (ip < addr + size) {
        nextip = disasm(__builtin_ia16_near_data_segment(), ip);
        ip = nextip;
    }
}

char buf[32767];

int disasm_file(char *filename)
{
    int fd, n;
    struct stat sbuf;

    if ((fd = open(filename, O_RDONLY)) < 0) {
        perror(filename);
        return 1;
    }
    if (fstat(fd, &sbuf) < 0) {
        perror("fstat");
        return 1;
    }
    if (sbuf.st_size > 32767) {
        fprintf(stderr, "Can't disassemble file > 32k bytes\n");
        close(fd);
        return 1;
    }
    if ((n = read(fd, buf, (ssize_t)sbuf.st_size)) != (int)sbuf.st_size) {
        perror("read");
        close(fd);
        return 1;
    }
    close(fd);
    disasm_buf(buf, n);
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
        disasm_mem((int)seg, (void *)(int)off, (int)count);
    } else return disasm_file(av[1]);
    return 0;
}
