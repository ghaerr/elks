#ifndef __LINUX_CONFIG_H__
#define __LINUX_CONFIG_H__

#include <linuxmt/autoconf.h>

#define REGOPT register

/*
 * Determine which root device selected
 */

#ifdef CONFIG_ROOTDEV_FD1
#define CONFIG_ROOTDEV 0x03c0
#endif

#ifdef CONFIG_ROOTDEV_RAM
#define CONFIG_ROOTDEV 0x0100
#endif

#ifdef CONFIG_ROOTDEV_BDA1
#define CONFIG_ROOTDEV 0x0301
#endif

#ifdef CONFIG_ROOTDEV_BDA2
#define CONFIG_ROOTDEV 0x0302
#endif

#ifdef CONFIG_ROOTDEV_BDA3
#define CONFIG_ROOTDEV 0x0303
#endif

#ifdef CONFIG_ROOTDEV_BDA4
#define CONFIG_ROOTDEV 0x0304
#endif

/*
 * If nothing was set, use the default of /dev/fd0
 */
#ifndef CONFIG_ROOTDEV
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
