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
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linuxmt/minix.h>
#include "debug/syms.h"

static unsigned char __far *syms;
struct minix_exec_hdr sym_hdr;

#define MAGIC       0x0301  /* magic number for executable progs */

static void noinstrument fmemcpy(unsigned char __far *dst, unsigned char *src, int n)
{
    do {
        *dst++ = *src++;
    } while (--n);
}

/* allocate space and read symbol table */
static unsigned char __far * noinstrument alloc_read(int fd, size_t size)
{
    if (size == 0 || size > 32767)
        return NULL;
#if __ia16__
    unsigned char __far *s;
    size_t n, t = 0;
    unsigned char buf[512];
    if (!(s = fmemalloc(size)))
        return NULL;
    do {
        n = size > sizeof(buf)? sizeof(buf): size;
        if (read(fd, buf, n) != n)
            return NULL;            // FIXME no fmemfree
        fmemcpy(s+t, buf, n);
        t += n;
        size -= n;
    } while (size);
    return s;
#else
    unsigned char *s;
    if (!(s = malloc(size)))
        return NULL;
    if (read(fd, s, size) != size)
        return NULL;
    return (unsigned char __far *)s;
#endif
}

/* read symbol table from executable into memory */
unsigned char __far * noinstrument sym_read_exe_symbols(char *path)
{
    int fd;
    unsigned char __far *s;
    char fullpath[PATH_MAX];

    if (syms) return syms;
    if ((fd = open(path, O_RDONLY)) < 0) {
        strcpy(fullpath, "/bin/");          // FIXME use path
        strcpy(fullpath+5, path);
        if ((fd = open(fullpath, O_RDONLY)) < 0)
                return NULL;
    }
    errno = 0;
    if (read(fd, &sym_hdr, sizeof(sym_hdr)) != sizeof(sym_hdr)
        || ((sym_hdr.type & 0xFFFF) != MAGIC)
        || (lseek(fd, -(int)sym_hdr.syms, SEEK_END) < 0)
        || !(s = alloc_read(fd, (size_t)sym_hdr.syms))) {
            int e = errno;
            close(fd);
            errno = e;
            return NULL;
    }
    close(fd);
    syms = s;
    return syms;
}

/* read symbol table file into memory */
unsigned char __far * noinstrument sym_read_symbols(char *path)
{
    int fd;
    unsigned char __far *s;
    struct stat sbuf;

    if (syms) return syms;
    if ((fd = open(path, O_RDONLY)) < 0)
        return NULL;
    errno = 0;
    if (fstat(fd, &sbuf) < 0
        || !(s = alloc_read(fd, (size_t)sbuf.st_size))) {
            int e = errno;
            close(fd);
            errno = e;
            return NULL;
    }
    close(fd);
    syms = s;
    return syms;
}

static int noinstrument type_text(unsigned char __far *p)
{
    return (p[TYPE] == 'T' || p[TYPE] == 't' || p[TYPE] == 'W');
}

static int noinstrument type_ftext(unsigned char __far *p)
{
    return (p[TYPE] == 'F' || p[TYPE] == 'f');
}

static int noinstrument type_data(unsigned char __far *p)
{
    return (p[TYPE] == 'D' || p[TYPE] == 'd' ||
            p[TYPE] == 'B' || p[TYPE] == 'b' ||
            p[TYPE] == 'V');
}

static char * noinstrument hexout(char *buf, unsigned int v, int zstart)
{
    int sh = 12;

    do {
        int c = (v >> sh) & 15;
        if (!c && sh > zstart)
            continue;
        if (c > 9)
            *buf++ = 'a' - 10 + c;
        else
            *buf++ = '0' + c;
    } while ((sh -= 4) >= 0);
    *buf = '\0';
    return buf;
}

static char * noinstrument hex(char *buf, unsigned int v)
{
    return hexout(buf, v, 0);   /* %x */
}

static char * noinstrument hex4(char *buf, unsigned int v)
{
    return hexout(buf, v, 12);  /* %04x */
}

static char * noinstrument fstrncpy(char *dst, unsigned char __far *src, int n)
{
    do {
        *dst++ = *src++;
    } while (--n);
    *dst = '\0';
    return dst;
}

/*
 * Convert address to symbol string+offset,
 * offset = 0 don't show offset, offset = -1 no offset specified.
 */
static char * noinstrument sym_string(void *addr, int offset,
    int (*istype)(unsigned char __far *p))
{
    unsigned char __far *p;
    unsigned char __far *lastp;
    char *cp;
    static char buf[64];

    if (!syms) {
hex:
        if (!offset || offset == -1) {
            hex4(buf, (size_t)addr);
        } else {
            cp = hex4(buf, (size_t)addr-offset);
            *cp++ = '+';
            hex(cp, offset);
        }
        return buf;
    }

    lastp = syms;
    while (!istype(lastp)) {
        lastp = next(lastp);
        if (!lastp[TYPE])
            goto hex;
    }
    for (p = next(lastp); ; lastp = p, p = next(p)) {
        if (!istype(p) || ((unsigned short)addr < *(unsigned short __far *)(&p[ADDR])))
            break;
    }
    int lastaddr = *(unsigned short __far *)(&lastp[ADDR]);
    if (offset && addr - lastaddr) {
        cp = fstrncpy(buf, lastp+SYMBOL, lastp[SYMLEN]);
        *cp++ = '+';
        hex(cp, (size_t)addr - lastaddr);
    } else {
        fstrncpy(buf, lastp+SYMBOL, lastp[SYMLEN]);
    }
    return buf;
}

/* convert .text address to symbol */
char * noinstrument sym_text_symbol(void *addr, int offset)
{
    return sym_string(addr, offset, type_text);
}

/* convert .fartext address to symbol */
char * noinstrument sym_ftext_symbol(void *addr, int offset)
{
    return sym_string(addr, offset, type_ftext);
}

/* convert .data address to symbol */
char * noinstrument sym_data_symbol(void *addr, int offset)
{
    return sym_string(addr, offset, type_data);
}

#if UNUSED
/* map .text address to function start address */
void * noinstrument sym_fn_start_address(void *addr)
{
    unsigned char __far *p;
    unsigned char __far *lastp;

    if (!syms) return NULL;
    lastp = syms;
    for (p = next(lastp); ; lastp = p, p = next(p)) {
        if (!type_text(p) || ((unsigned short)addr < *(unsigned short __far *)(&p[ADDR])))
            break;
    }
    return (void *) (intptr_t) *(unsigned short __far *)(&lastp[ADDR]);
}

static int noinstrument type_any(unsigned char *p)
{
    return p[TYPE] != '\0';
}

/* convert (non-segmented local IP) address to symbol */
char * noinstrument sym_symbol(void *addr, int offset)
{
    return sym_string(addr, offset, type_any);
}
#endif
