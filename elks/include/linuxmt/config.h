#ifndef _LINUX_CONFIG_H
#define _LINUX_CONFIG_H

#include <linuxmt/autoconf.h>

#define REGOPT register

/*
 * Set this to 0x380 to have /dev/fd0 be the root device, or 0x3c0 for /dev/fd1 
 */

#ifndef CONFIG_ROOTDEV
/* 
 * 0x0380 /dev/fd0   bios floppy
 * 0x0100 /dev/ram0  ramdisk ram0      
 * 0x0301 /dev/bda1  bios disk0, part 1
 */
#define CONFIG_ROOTDEV 0x0380
#endif

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
#define UTS_NODENAME "(none)"	/* set by sethostname() */
#endif

#ifndef UTS_DOMAINNAME
#define UTS_DOMAINNAME "(none)"	/* set by setdomainname() */
#endif

/*
 * The definitions for UTS_RELEASE and UTS_VERSION are now defined
 * in linux/version.h, and should only be used by linux/version.c
 */

/* Don't touch these, unless you really know what your doing. */
#define DEF_INITSEG	0x0100   /* original 0x0100
				  * for netboot use for example 
				  * 0x5000, as 0x0100 cannot be used
				  * in connection to the netboot
				  * loader
				  */
#define DEF_SYSSEG	0x1000
#define DEF_SETUPSEG	DEF_INITSEG + 0x20
#define DEF_SYSSIZE	0x2F00

/* internal svga startup constants */
#define NORMAL_VGA	0xffff		/* 80x25 mode */
#define EXTENDED_VGA	0xfffe		/* 80x50 mode */
#define ASK_VGA		0xfffd		/* ask for it at bootup */

#endif
