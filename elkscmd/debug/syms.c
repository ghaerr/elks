/*
 * ELKS symbol table support
 *
 * July 2022 Greg Haerr
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdint.h>
#include "syms.h"

static unsigned char *syms;

#if __ia16__
#define ALLOC(s,n)    ((int)(s = sbrk(n)) != -1)
#else
#define ALLOC(s,n)    ((s = malloc(n)) !=  NULL)
char * _program_filename;
#endif

struct minix_exec_hdr {
    uint32_t  type;
    uint8_t   hlen;       // 0x04
    uint8_t   reserved1;
    uint16_t  version;
    uint32_t  tseg;       // 0x08
    uint32_t  dseg;       // 0x0c
    uint32_t  bseg;       // 0x10
    uint32_t  entry;
    uint16_t  chmem;
    uint16_t  minstack;
    uint32_t  syms;
};

/* read symbol table from executable into memory */
unsigned char * noinstrument sym_read_exe_symbols(char *path)
{
    int fd;
    unsigned char *s;
    struct minix_exec_hdr hdr;

    if (syms) return syms;
    if ((fd = open(path, O_RDONLY)) < 0)
        return NULL;
    errno = 0;
    if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)
        || (hdr.syms == 0 || hdr.syms > 32767)
        || (!ALLOC(s, (int)hdr.syms))
        || (lseek(fd, -(int)hdr.syms, SEEK_END) < 0)
        || (read(fd, s, (int)hdr.syms) != (int)hdr.syms)) {
                int e = errno;
                close(fd);
                errno = e;
                return NULL;
    }
    syms = s;
    return syms;
}

/* read symbol table file into memory */
unsigned char * noinstrument sym_read_symbols(char *path)
{
    int fd;
    unsigned char *s;
    struct stat sbuf;

    if (syms) return syms;
    if ((fd = open(path, O_RDONLY)) < 0)
        return NULL;
    errno = 0;
    if (fstat(fd, &sbuf) < 0
        || (sbuf.st_size == 0 || sbuf.st_size > 32767)
        || (!ALLOC(s, (int)sbuf.st_size))
        || (read(fd, s, (int)sbuf.st_size) != (int)sbuf.st_size)) {
                int e = errno;
                close(fd);
                errno = e;
                return NULL;
    }
    syms = s;
    return syms;
}

static int noinstrument type_text(unsigned char *p)
{
    return (p[TYPE] == 'T' || p[TYPE] == 't');
}

static int noinstrument type_ftext(unsigned char *p)
{
    return (p[TYPE] == 'F' || p[TYPE] == 'f');
}

static int noinstrument type_data(unsigned char *p)
{
    return (p[TYPE] == 'D' || p[TYPE] == 'd' ||
            p[TYPE] == 'B' || p[TYPE] == 'b');
}

/* map .text address to function start address */
void * noinstrument sym_fn_start_address(void *addr) 
{
    unsigned char *p, *lastp;

    if (!syms && !sym_read_exe_symbols(_program_filename)) return (void *)-1;

    lastp = syms;
    for (p = next(lastp); ; lastp = p, p = next(p)) {
        if (!type_text(p) || ((unsigned short)addr < *(unsigned short *)(&p[ADDR])))
            break;
    }
    return (void *) (intptr_t) *(unsigned short *)(&lastp[ADDR]);
}

/* convert address to symbol string */
static char * noinstrument sym_string(void *addr, int exact,
    int (*istype)(unsigned char *p))
{
    unsigned char *p, *lastp;
    static char buf[32];

    if (!syms && !sym_read_exe_symbols(_program_filename)) {
hex:
        sprintf(buf, "%.4x", (unsigned int)addr);
        return buf;
    }

    lastp = syms;
    while (!istype(lastp)) {
        lastp = next(lastp);
        if (!lastp[TYPE])
            goto hex;
    }
    for (p = next(lastp); ; lastp = p, p = next(p)) {
        if (!istype(p) || ((unsigned short)addr < *(unsigned short *)(&p[ADDR])))
            break;
    }
    int lastaddr = *(unsigned short *)(&lastp[ADDR]);
    if (exact && addr - lastaddr) {
        sprintf(buf, "%.*s+%xh", lastp[SYMLEN], lastp+SYMBOL,
                                (unsigned int)addr - lastaddr);
    } else sprintf(buf, "%.*s", lastp[SYMLEN], lastp+SYMBOL);
    return buf;
}

/* convert .text address to symbol */
char * noinstrument sym_text_symbol(void *addr, int exact)
{
    return sym_string(addr, exact, type_text);
}

/* convert .fartext address to symbol */
char * noinstrument sym_ftext_symbol(void *addr, int exact)
{
    return sym_string(addr, exact, type_ftext);
}

/* convert .data address to symbol */
char * noinstrument sym_data_symbol(void *addr, int exact)
{
    return sym_string(addr, exact, type_data);
}
