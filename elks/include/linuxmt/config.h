#ifndef __LINUXMT_CONFIG_H
#define __LINUXMT_CONFIG_H

#include <autoconf.h>
#include <linuxmt/major.h>

#define REGOPT register

/*
 * Use this option to download a ramdisk (/dev/ram0) with the netboot
 * protocol. This ramdisk can be used as mini root disk (128KB) or
 * as auxilary disk.
 */
#undef CONFIG_PRELOAD_RAMDISK

/* 
 * The ramdisk is placed in the seg 0x6000 and 0x7000. So the ram is 
 * reduced to 384KB main memory. This is only for test and experimental
 * use.
 */
#ifdef CONFIG_PRELOAD_RAMDISK
#define RAM_REDUCE 0x4000L
#else
#define RAM_REDUCE 0x0000L
#endif

/*
 * Defines for what uname() should return 
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

#ifndef UTS_DOMAINNAME
#define UTS_DOMAINNAME "(none)" 	/* set by setdomainname() */
#endif

/*
 * The definitions for UTS_RELEASE and UTS_VERSION are now defined
 * in linux/version.h, and should only be used by linux/version.c
 */

/* Don't touch these, unless you really know what you are doing. */
#define DEF_INITSEG	0x0100	/* original 0x0100
				 * for netboot use for example 0x5000, as
				 * 0x0100 cannot be used in connection
				 * with the netboot loader
				 */
#define DEF_SYSSEG	0x1000
#define DEF_SETUPSEG	DEF_INITSEG + 0x20
#define DEF_SYSSIZE	0x2F00
#define DEF_OPTSEG	0x50

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

/* internal svga startup constants */
#define VGA_NORMAL	0xffff	/* 80x25 mode */
#define VGA_EXTENDED	0xfffe	/* 80x50 mode */
#define VGA_ASK 	0xfffd	/* ask for it at bootup */

#endif
