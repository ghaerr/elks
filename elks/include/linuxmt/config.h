#ifndef _LINUX_CONFIG_H
#define _LINUX_CONFIG_H

#include <linuxmt/autoconf.h>

#define REGOPT 

/*
 * Set this to 0x380 to have /dev/fd0 be the root device, or 0x3c0 for /dev/fd1 
 */

#ifndef CONFIG_ROOTDEV
#define CONFIG_ROOTDEV 0x380
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
#define DEF_INITSEG	0x0100
#define DEF_SYSSEG	0x1000
#define DEF_SETUPSEG	0x0120
#define DEF_SYSSIZE	0x2F00

/* internal svga startup constants */
#define NORMAL_VGA	0xffff		/* 80x25 mode */
#define EXTENDED_VGA	0xfffe		/* 80x50 mode */
#define ASK_VGA		0xfffd		/* ask for it at bootup */

#endif
