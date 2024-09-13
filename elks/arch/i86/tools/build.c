/*
 *  linux/tools/build.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *  Hacked (H) 1995 Chad Page for use with building MINIX a.out files
 */

/*
 * This file builds the /linux system image from two different files:
 *
 * setup: setup.S assembled to a.out format, sets up system parms
 * system: compiled kernel in a.out format
 *
 * It does some checking that all files are of the correct type, and
 * just writes the result to stdout, removing headers and padding to
 * the right amount. It also writes some system data to stderr.
 */

#include <stdio.h>                      /* fprintf */
#include <sys/types.h>                  /* unistd.h needs this */
#include <string.h>
#include <stdlib.h>                     /* contains exit */
#include <unistd.h>                     /* contains read/write */
#include <fcntl.h>
#include <stdint.h>

#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/boot.h>

#include "a.out.h"

#define MINIX_HEADER 0x20
#define SUPL_HEADER  0x40

#define DEFAULT_MAJOR_ROOT 0x03         /* BIOS floppy 0 */
#define DEFAULT_MINOR_ROOT 0x80

/* max nr of sectors of setup: don't change unless you also change bootsect etc
 */
#define SETUP_SECTS 4

#define STRINGIFY(x) #x

typedef union {
    uint32_t l;
    uint16_t s[2];
    uint8_t b[4];
} conv;

int32_t intel_long(int32_t l)
{
    conv t;

    t.b[0] = l & 0xff;
    l >>= 8;
    t.b[1] = l & 0xff;
    l >>= 8;
    t.b[2] = l & 0xff;
    l >>= 8;
    t.b[3] = l & 0xff;
    l >>= 8;
    return t.l;
}

int16_t intel_short(int16_t l)
{
    conv t;

    t.b[0] = l & 0xff;
    l >>= 8;
    t.b[1] = l & 0xff;
    l >>= 8;
    return t.s[0];
}

void die(const char *str)
{
    fprintf(stderr, "%s\n", str);
    exit(1);
}

void usage(void)
{
    die("Usage: build bootsect setup system [> image]");
}

int main(int argc, char **argv)
{
    int32_t i, c, id, sz, fsz;
    uint32_t sys_size;
    unsigned char major_root, minor_root;
    unsigned char setup_sectors;
    char buf[1024];
    struct exec *ex = (struct exec *) buf;

    if (argc != 4)
        usage();
    major_root = DEFAULT_MAJOR_ROOT;
    minor_root = DEFAULT_MINOR_ROOT;
    fprintf(stderr, "Root device is (%d, %d)\n", major_root, minor_root);
    for (i = 0; i < sizeof buf; i++)
        buf[i] = 0;
#ifdef USEDUMMYBOOT
    if ((id = open(argv[1], O_RDONLY, 0)) < 0)
        die("Unable to open 'boot'");
    if (read(id, buf, MINIX_HEADER) != MINIX_HEADER)
        die("Unable to read header of 'boot'");
    if (((uint32_t *) buf)[0] != intel_long(0x04100301))
        die("Non-Minix header of 'boot'");
    if (((uint32_t *) buf)[1] != intel_long(MINIX_HEADER))
        die("Non-Minix header of 'boot'");
    if (((uint32_t *) buf)[3] != 0)
        die("Illegal data segment in 'boot'");
    if (((uint32_t *) buf)[4] != 0)
        die("Illegal bss in 'boot'");
    if (((uint32_t *) buf)[5] != 0)
        die("Non-Minix header of 'boot'");
    if (((uint32_t *) buf)[7] != 0)
        die("Illegal symbol table in 'boot'");
    i = read(id, buf, sizeof buf);
    fprintf(stderr, "Boot sector %d bytes.\n", i);
    if (i != 512)
        die("Boot block must be exactly 512 bytes");
    if ((*(unsigned short *) (buf + 510)) !=
        (unsigned short) intel_short(0xAA55))
            die("Boot block hasn't got boot flag (0xAA55)");
    close(id);
#endif
    buf[root_dev] = (char) minor_root;          /* WRITE root_dev*/
    buf[root_dev+1] = (char) major_root;
    i = write(1, buf, 512);                     /* this sector becomes INITSEG */
    if (i != 512)
        die("Write call failed");

    if ((id = open(argv[2], O_RDONLY, 0)) < 0)
        die("Unable to open 'setup'");
    if (read(id, buf, MINIX_HEADER) != MINIX_HEADER)
        die("Unable to read header of 'setup'");
    if (((uint32_t *) buf)[0] != intel_long(0x04100301))
        die("Non-Minix header of 'setup'");
    if (((uint32_t *) buf)[1] != intel_long(MINIX_HEADER))
        die("Non-Minix header of 'setup'");
    if (((uint32_t *) buf)[3] != 0)
        die("Illegal data segment in 'setup'");
    if (((uint32_t *) buf)[4] != 0)
        die("Illegal bss in 'setup'");
    if (((uint32_t *) buf)[5] != 0)
        die("Non-Minix header of 'setup'");
    if (((uint32_t *) buf)[7] != 0)
        die("Illegal symbol table in 'setup'");
    for (i = 0; (c = read(id, buf, sizeof buf)) > 0; i += c)
        if (write(1, buf, c) != c)
            die("Write call failed");
    if (c != 0)
        die("read-error on 'setup'");
    close(id);
    setup_sectors = (unsigned char) ((i + 511) / 512);
    /* for compatibility with LILO */
    if (setup_sectors < SETUP_SECTS)
        setup_sectors = SETUP_SECTS;
    fprintf(stderr, "Setup is %d bytes (%d sectors).\n", i, setup_sectors);
    for (c = 0; c < sizeof(buf); c++)
        buf[c] = '\0';
    while (i < setup_sectors * 512) {
        c = setup_sectors * 512 - i;
        if (c > sizeof(buf))
            c = sizeof(buf);
        if (write(1, buf, c) != c)
            die("Write call failed");
        i += c;
    }

    if ((id = open(argv[3], O_RDONLY, 0)) < 0)
        die("Unable to open 'system'");

    if (read(id, buf, SUPL_HEADER) != SUPL_HEADER)
        die("Unable to read header of 'system'");
    if ((ex->a_magic[0] != 0x01) || (ex->a_magic[1] != 0x03)) {
        die("Non-MINIX header of 'system'");
    }
    fsz = 0;
    sz = intel_long(ex->a_text) + intel_long(ex->a_data) + intel_long(ex->a_bss);
    if (ex->a_hdrlen == SUPL_HEADER) {
        fsz = intel_long(ex->esh_ftseg);
        sz += fsz;
    }
    fprintf(stderr, "Kernel is %d B (%d B code, %d B fartext, %d B data and %u B bss)\n",
        sz, intel_long(ex->a_text), fsz,
        intel_long(ex->a_data), (unsigned) intel_long(ex->a_bss));
    sz = ex->a_hdrlen + intel_long(ex->a_text) + intel_long(ex->a_data);
    if (ex->a_hdrlen == SUPL_HEADER) {
            sz += fsz;
            sz += intel_long(ex->a_trsize) + intel_long(ex->a_drsize)
               + intel_long(ex->esh_ftrsize);
    }
    if (intel_long(ex->a_data) + intel_long(ex->a_bss) > 65535) {
        fprintf(stderr, "Image too large.\n");
        exit(1);
    }

    lseek(id, 0, 0);

    sys_size = (sz + 15) / 16;
    fprintf(stderr, "System is %d B (0x%x paras)\n", sz, sys_size);
    if (sys_size > DEF_SYSMAX)
        die("System is too big");
#ifndef CONFIG_ROMCODE
    /* display start and end setup.S copy address for informational purposes */
    fsz = (ex->a_hdrlen + (intel_long(ex->a_text) + intel_long(ex->a_data)) + 15) / 16;
    if (ex->a_hdrlen == SUPL_HEADER) {
        fsz += (intel_long(ex->a_trsize) + intel_long(ex->a_drsize)
            + intel_long(ex->esh_ftrsize) + intel_long(ex->esh_ftseg) + 15) / 16;
    }
    fsz += REL_SYSSEG;
    fprintf(stderr, "Load segment start 0x%x end 0x%x\n", DEF_SYSSEG, fsz);
    if (DEF_SYSSEG < REL_SYSSEG + 0x1000) {
        fprintf(stderr, "Warning: Load segment too low for REL_SYSSEG at %x, increase DEF_SYSSEG\n", REL_SYSSEG);
    }
#endif
    while (sz > 0) {
        int32_t l, n;

        l = sz;
        if (l > sizeof(buf))
            l = sizeof(buf);
        if ((n = read(id, buf, l)) != l) {
            if (n == -1)
                perror(argv[1]);
            else
                fprintf(stderr, "Unexpected EOF\n");
            die("Can't read 'system'");
        }
        if (write(1, buf, l) != l)
            die("Write failed");
        sz -= l;
    }
    close(id);
    if (lseek(1, elks_magic, 0) == elks_magic) {
        if (write(1, "ELKS", 4) != 4)                   /* WRITE elks_magic */
            die("Write failed");
    }
    if (lseek(1, setup_sects, 0) == setup_sects) {
        if (write(1, &setup_sectors, 1) != 1)           /* WRITE setup_sectors*/
            die("Write of setup sectors failed");
    }
    if (lseek(1, syssize, 0) == syssize) {
        buf[0] = (sys_size & 0xff);                     /* WRITE sys_size*/
        buf[1] = ((sys_size >> 8) & 0xff);
        if (write(1, buf, 2) != 2)
            die("Write failed");
    }
    return 0;
}
