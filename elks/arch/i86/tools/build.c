/*
 *  linux/tools/build.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *  Hacked (H) 1995 Chad Page for use with building MINIX a.out files
 */

/*
 * This file builds a disk-image from three different files:
 *
 * - bootsect: exactly 512 bytes of 8086 machine code, loads the rest
 * - setup: 8086 machine code, sets up system parm
 * - system: 8086 code for actual system
 *
 * It does some checking that all files are of the correct type, and
 * just writes the result to stdout, removing headers and padding to
 * the right amount. It also writes some system data to stderr.
 */

/*
 * Changes by tytso to allow root device specification
 */

#include <stdio.h>			/* fprintf */

#ifdef __linux__
#include <sys/sysmacros.h>		/* for major/minor macros */
#endif
#include <sys/stat.h>
#include <sys/types.h>			/* unistd.h needs this */

#include <string.h>
#include <stdlib.h>			/* contains exit */
#include <unistd.h>			/* contains read/write */
#include <fcntl.h>
#include <stdint.h>

#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/boot.h>

#include "a.out.h"

#define MINIX_HEADER 0x20
#define SUPL_HEADER  0x40

#define N_MAGIC_OFFSET 1024

#ifndef __BFD__
static int GCC_HEADER = sizeof(struct exec);
#endif

#define SYS_SIZE DEF_SYSSIZE

#define DEFAULT_MAJOR_ROOT 0x03
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
    die("Usage: build bootsect setup system [rootdev] [> image]");
}

int main(int argc, char **argv)
{
    int32_t i, c, id, sz, fsz;
    uint32_t sys_size;
    char buf[1024];

#ifndef __BFD__
    struct exec *ex = (struct exec *) buf;
#endif

    unsigned char major_root, minor_root;
    struct stat sb;
    unsigned char setup_sectors;

#define ROOTDEV_ARG 4

    if ((argc < ROOTDEV_ARG) || (argc > ROOTDEV_ARG + 1))
	usage();
    if (argc > ROOTDEV_ARG) {
	if (!strcmp(argv[ROOTDEV_ARG], "CURRENT")) {
	    if (stat("/", &sb)) {
		perror("/");
		die("Couldn't stat /");
	    }
	    major_root = major(sb.st_dev);
	    minor_root = minor(sb.st_dev);
	} else if (strcmp(argv[ROOTDEV_ARG], "FLOPPY")) {
	    if (stat(argv[ROOTDEV_ARG], &sb)) {
		perror(argv[ROOTDEV_ARG]);
		die("Couldn't stat root device.");
	    }
	    major_root = major(sb.st_rdev);
	    minor_root = minor(sb.st_rdev);
	} else {
	    major_root = 0;
	    minor_root = 0;
	}
    } else {
	major_root = DEFAULT_MAJOR_ROOT;
	minor_root = DEFAULT_MINOR_ROOT;
    }
    fprintf(stderr, "Root device is (%d, %d)\n", major_root, minor_root);
    for (i = 0; i < sizeof buf; i++)
	buf[i] = 0;
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
    buf[root_dev] = (char) minor_root;		/* WRITE root_dev*/
    buf[root_dev+1] = (char) major_root;
    i = write(1, buf, 512);
    if (i != 512)
	die("Write call failed");
    close(id);

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
    fprintf(stderr, "Setup is %d bytes.\n", i);
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
#ifndef __BFD__
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
    fprintf(stderr, "System is %d B (%d B code, %d B fartext, %d B data and %u B bss)\n",
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
#else
    if (fstat(id, &sb)) {
	perror("fstat");
	die("Unable to stat 'system'");
    }
    sz = sb.st_size;
    fprintf(stderr, "System is %d kB\n", sz / 1024);
#endif
    sys_size = (sz + 15) / 16;
    fprintf(stderr, "System is %d (%xh paras)\n", sz, sys_size);
    if (sys_size > SYS_SIZE)
	die("System is too big");
#ifndef CONFIG_ROMCODE
    /* compute boot load address to avoid overwriting image at boot relocation time*/
    fsz = (ex->a_hdrlen + (intel_long(ex->a_text) + intel_long(ex->a_data)) + 15) / 16
	+ REL_SYSSEG;
    fprintf(stderr, "Text/Data load ending segment is %x, limit %x\n", fsz, DEF_SYSSEG);
    if (fsz > DEF_SYSSEG + REL_SYSSEG) {	/* start of reloc entry table at boot */
	fprintf(stderr, "System too large for DEF_SYSSEG at %x, increase DEF_SYSSEG\n",
	    DEF_SYSSEG);
	exit(1);
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
    if (lseek(1, setup_sects, 0) == setup_sects) {
	if (write(1, &setup_sectors, 1) != 1)		/* WRITE setup_sectors*/
	    die("Write of setup sectors failed");
    }
    if (lseek(1, syssize, 0) == syssize) {
	buf[0] = (sys_size & 0xff);			/* WRITE sys_size*/
	buf[1] = ((sys_size >> 8) & 0xff);
	if (write(1, buf, 2) != 2)
	    die("Write failed");
    }
    return 0;
}
