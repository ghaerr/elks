#ifndef __LINUXMT_CONFIG_H
#define __LINUXMT_CONFIG_H

#include <autoconf.h>
#include <linuxmt/major.h>

/* Don't touch these, unless you really know what you are doing. */
#define DEF_INITSEG	0x0100	/* original 0x0100
				 * for netboot use for example 0x5000, as
				 * 0x0100 cannot be used in connection
				 * with the netboot loader
				 */
#define DEF_SYSSEG	0x1000
#define DEF_SETUPSEG	DEF_INITSEG + 0x20
#define DEF_SYSSIZE	0x2F00
#define DEF_OPTSEG	0x50  /* /bootopts loaded at this segment*/

#if !defined(CONFIG_ROMCODE) && !defined(CONFIG_ARCH_SIBO)
#define REL_SYS
#define REL_INITSEG	0x60  /* future CONFIG_MEM_LOW */
#define REL_SYSSEG	0xC0  /* after DMASEG */
#endif

// DMASEG is a bouncing buffer of 1K (= BLOCKSIZE)
// below the first 64K boundary (= 0x1000:0)
// for use with the old 8237 DMA controller

#if !defined(CONFIG_ARCH_SIBO)
#define DMASEG 0x80    /* after REL_INITSEG */
#define DMASEGSZ 1024  /* BLOCK_SIZE */
#else
#define DMASEG 0x800
#endif

#if defined(REL_SYS)
#define SETUP_DATA REL_INITSEG
#elif defined(CONFIG_ROMCODE)
#define SETUP_DATA CONFIG_ROM_SETUP_DATA
#else
#define SETUP_DATA DEF_INITSEG
#endif

/*
 * Defines for what uname() should return.
 * The definitions for UTS_RELEASE and UTS_VERSION are now passed as
 * kernel compilation parameters, and should only be used by linux/version.c
 */
#ifndef UTS_SYSNAME
#define UTS_SYSNAME "ELKS"
#endif

#ifndef UTS_MACHINE
#define UTS_MACHINE "i8086"
#endif

#ifndef UTS_NODENAME
#define UTS_NODENAME "(none)"		/* set by sethostname() */
#endif

/* internal svga startup constants */
#define VGA_NORMAL	0xffff	/* 80x25 mode */
#define VGA_EXTENDED	0xfffe	/* 80x50 mode */
#define VGA_ASK 	0xfffd	/* ask for it at bootup */

#endif
