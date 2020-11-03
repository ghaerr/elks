#ifndef __LINUXMT_CONFIG_H
#define __LINUXMT_CONFIG_H

#include <autoconf.h>
#include <linuxmt/major.h>

/* Don't touch these, unless you really know what you are doing. */
#define DEF_INITSEG	0x0100	/* setup data, for netboot use 0x5000 */
#define DEF_SYSSEG	0x1000
#define DEF_SETUPSEG	DEF_INITSEG + 0x20
#define DEF_SYSSIZE	0x2F00

#ifdef CONFIG_ROMCODE
#define SETUP_DATA	CONFIG_ROM_SETUP_DATA

#else
#define SETUP_DATA	REL_INITSEG

/* Define segment locations of low memory, must not overlap */
#define DEF_OPTSEG	0x50  /* 0x100 bytes boot options*/
#define REL_INITSEG	0x60  /* 0x200 bytes setup data */
#define DMASEG		0x80  /* 0x400 bytes floppy sector buffer */

#ifdef CONFIG_TRACK_CACHE     /* floppy track buffer in low mem */
#define DMASEGSZ 0x2400	      /* SECTOR_SIZE * 18 (9216) */
#define REL_SYSSEG	0x2C0 /* kernel code segment */
#else
#define DMASEGSZ 0x0400	      /* BLOCK_SIZE (1024) */
#define REL_SYSSEG	0x0C0 /* kernel code segment */
#endif

#endif /* !CONFIG_ROMCODE */

// DMASEG is a bouncing buffer of 1K (= BLOCKSIZE)
// below the first 64K boundary (= 0x1000:0)
// for use with the old 8237 DMA controller
// OR a floppy track buffer of 9K (18 512-byte sectors)

#ifdef CONFIG_ARCH_SIBO
#define DMASEG 0x800
#endif

/*
 * Defines for what uname() should return.
 * The definitions for UTS_RELEASE and UTS_VERSION are now passed as
 * kernel compilation parameters, and should only be used by linux/version.c
 */
#define UTS_SYSNAME "ELKS"
#define UTS_MACHINE "i8086"
#define UTS_NODENAME "(none)"		/* set by sethostname() */

#endif
