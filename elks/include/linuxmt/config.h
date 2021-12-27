#ifndef __LINUXMT_CONFIG_H
#define __LINUXMT_CONFIG_H

#include <autoconf.h>
#include <linuxmt/major.h>

/* tunable parameters*/
#define PIPE_BUFSIZ	80	/* doesn't have to be power of two */

#ifdef CONFIG_IMG_FD1232
#define SECTOR_SIZE     1024	/* sector size (bytes) */
#define SECTOR_BITS	10	/* log2(SECTOR_SIZE) */
#else
#define SECTOR_SIZE     512	/* sector size (bytes) */
#define SECTOR_BITS	9	/* log2(SECTOR_SIZE) */
#endif

/*
 * Compile-time configuration
 */

#ifdef CONFIG_ARCH_IBMPC
#define MAX_SERIAL		4		/* max number of serial tty devices*/

/*
 * Setup data - normally queried by startup setup.S code, but can
 * be overridden for embedded systems with less overhead.
 * See setup.S for more details.
 */
#define SETUP_VID_COLS		setupb(7)	/* BIOS video # columns */
#define SETUP_VID_LINES		setupb(14)	/* BIOS video # lines */
#define SETUP_CPU_TYPE		setupb(0x20)	/* processor type */
#define SETUP_MEM_KBYTES	setupw(0x2a)	/* base memory in 1K bytes */
#define SETUP_ROOT_DEV		setupw(0x1fc)	/* root device, kdev_t or BIOS dev */
#define SETUP_ELKS_FLAGS	setupw(0x1f6)	/* flags for root device type */
#define SETUP_PART_OFFSETLO	setupw(0x1e2)	/* partition offset low word */
#define SETUP_PART_OFFSETHI	setupw(0x1e4)	/* partition offset high word */
#ifdef CONFIG_ROMCODE
#define SYS_CAPS	(CAP_PC_AT|CAP_DRIVE_PARMS)
#endif
#endif

#ifdef CONFIG_ARCH_PC98
#define MAX_SERIAL		2	/* max number of serial tty devices*/
#define SETUP_VID_COLS		80	/* video # columns */
#define SETUP_VID_LINES		25	/* video # lines */
#define SETUP_CPU_TYPE		1	/* processor type = 8086 */
#define SETUP_MEM_KBYTES	640	/* base memory in 1K bytes */
#define SETUP_ROOT_DEV		setupw(0x1fc)	/* root device, kdev_t or BIOS dev */
#define SETUP_ELKS_FLAGS	setupw(0x1f6)	/* flags for root device type */
#define SETUP_PART_OFFSETLO	setupw(0x1e2)	/* partition offset low word */
#define SETUP_PART_OFFSETHI	setupw(0x1e4)	/* partition offset high word */
#define SYS_CAPS		0	/* no XT/AT capabilities */
#endif

#ifdef CONFIG_ARCH_8018X
#define MAX_SERIAL		2	/* max number of serial tty devices*/
#define SETUP_VID_COLS		80	/* video # columns */
#define SETUP_VID_LINES		25	/* video # lines */
#define SETUP_CPU_TYPE		5	/* processor type 80186 */
#define SETUP_MEM_KBYTES	512	/* base memory in 1K bytes */
#define SETUP_ROOT_DEV		0x0600	/* root device ROMFS */
#define SETUP_ELKS_FLAGS	0	/* flags for root device type */
#define SETUP_PART_OFFSETLO	0	/* partition offset low word */
#define SETUP_PART_OFFSETHI	0	/* partition offset high word */
#define SYS_CAPS		0	/* no XT/AT capabilities */


#define CONFIG_8018X_FCPU	16
#define CONFIG_8018X_EB
#endif

/*
 * System capabilities - configurable for ROM or custom installations.
 * Normally, all capabilities will be set if arch_cpu > 5 (PC/AT),
 * except when SYS_CAPS is defined for custom installations or emulations.
 */
#define CAP_PC_AT       0x01      /* PC/AT capabilities */
#define CAP_DRIVE_PARMS	0x02      /* has INT 13h fn 8 drive parms */
#define CAP_KBD_LEDS    0x04      /* has keyboard LEDs */
#define CAP_HD_IDE      0x08      /* can do hard drive IDE probes */
#define CAP_IRQ8TO15    CAP_PC_AT /* has IRQ 8 through 15 */
#define CAP_IRQ2MAP9    CAP_PC_AT /* map IRQ 2 to 9 */
#define CAP_ALL         0xFF      /* all capabilities if PC/AT only */

/* Don't touch these, unless you really know what you are doing. */
#define DEF_INITSEG	0x0100	/* initial Image load address by boot code */
#define DEF_SYSSEG	0x1300	/* kernel copied here by setup.S code */
#define DEF_SETUPSEG	DEF_INITSEG + 0x20
#define DEF_SYSSIZE	0x2F00

#ifdef CONFIG_ROMCODE
#ifdef CONFIG_BLK_DEV_BIOS    /* BIOS disk driver*/
#define DMASEG		0x80  /* 0x400 bytes floppy sector buffer */
#ifdef CONFIG_TRACK_CACHE     /* floppy track buffer in low mem */
#define DMASEGSZ 0x2400       /* SECTOR_SIZE * 18 (9216) */
#define KERNEL_DATA	0x2C0 /* kernel data segment */
#else
#define DMASEGSZ 0x040	      /* BLOCK_SIZE (1024) */
#define KERNEL_DATA	0x0C0 /* kernel data segment */
#endif
#else
#define KERNEL_DATA     0x80  /* kernel data segment */
#endif
#define SETUP_DATA	CONFIG_ROM_SETUP_DATA
#endif /* CONFIG_ROMCODE */


#if (defined(CONFIG_ARCH_IBMPC) || defined(CONFIG_ARCH_8018X)) && !defined(CONFIG_ROMCODE)
/* Define segment locations of low memory, must not overlap */
#define DEF_OPTSEG	0x50  /* 0x200 bytes boot options*/
#define OPTSEGSZ 0x200    /* max size of /bootopts file (1K max) */
#define REL_INITSEG	0x70  /* 0x200 bytes setup data */
#define DMASEG		0x90  /* 0x400 bytes floppy sector buffer */

#ifdef CONFIG_TRACK_CACHE     /* floppy track buffer in low mem */
#define DMASEGSZ 0x2400	      /* SECTOR_SIZE * 18 (9216) */
#define REL_SYSSEG	0x2D0 /* kernel code segment */
#else
#define DMASEGSZ 0x0400	      /* BLOCK_SIZE (1024) */
#define REL_SYSSEG	0x0D0 /* kernel code segment */
#endif
#define SETUP_DATA	REL_INITSEG
#endif /* (CONFIG_ARCH_IBMPC || CONFIG_ARCH_8018X) && !CONFIG_ROMCODE */

#if defined(CONFIG_ARCH_PC98) && !defined(CONFIG_ROMCODE)
/* Define segment locations of low memory, must not overlap */
#define DEF_OPTSEG	0x60  /* 0x200 bytes boot options*/
#define OPTSEGSZ 0x200    /* max size of /bootopts file (1K max) */
#define REL_INITSEG	0x80  /* 0x200 bytes setup data */
#define DMASEG		0xA0  /* 0x400 bytes floppy sector buffer */

#ifdef CONFIG_TRACK_CACHE     /* floppy track buffer in low mem */
#define DMASEGSZ 0x2000	      /* SECTOR_SIZE * 8 (8192) */
#define REL_SYSSEG	0x2A0 /* kernel code segment */
#else
#define DMASEGSZ 0x0400	      /* BLOCK_SIZE (1024) */
#define REL_SYSSEG	0x0E0 /* kernel code segment */
#endif
#define SETUP_DATA	REL_INITSEG
#endif /* CONFIG_ARCH_PC98 && !CONFIG_ROMCODE */


// DMASEG is a bouncing buffer of 1K (= BLOCKSIZE)
// below the first 64K boundary (= 0x1000:0)
// for use with the old 8237 DMA controller
// OR a floppy track buffer of 9K (18 512-byte sectors)

/*
 * Defines for what uname() should return.
 * The definitions for UTS_RELEASE and UTS_VERSION are now passed as
 * kernel compilation parameters, and should only be used by linux/version.c
 */
#define UTS_SYSNAME "ELKS"
#define UTS_MACHINE "i8086"
#define UTS_NODENAME "(none)"		/* set by sethostname() */

#endif
