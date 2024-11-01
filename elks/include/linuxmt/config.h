#ifndef __LINUXMT_CONFIG_H
#define __LINUXMT_CONFIG_H

#include <autoconf.h>
#include <linuxmt/major.h>

/*
 * Compile-time configuration
 */
#define UTS_SYSNAME "ELKS"                      /* uname system name */
#define UTS_NODENAME "elks"                     /* someday set by sethostname() */

#define CONFIG_MSDOS_PARTITION  1               /* support DOS HD partitions */
#define CONFIG_FS_DEV           1               /* support FAT /dev folder */

#ifdef CONFIG_ARCH_IBMPC
#define MAX_SERIAL              4               /* max number of serial tty devices*/

/*
 * Setup data - normally queried by startup setup.S code, but can
 * be overridden for embedded systems with less overhead.
 * See setup.S for more details.
 */
#define SETUP_VID_COLS          setupb(7)       /* BIOS video # columns */
#define SETUP_VID_LINES         setupb(14)      /* BIOS video # lines */
#define SETUP_CPU_TYPE          setupb(0x20)    /* processor type */
#define SETUP_MEM_KBYTES        setupw(0x2a)    /* base memory in 1K bytes */
#define SETUP_ROOT_DEV          setupw(0x1fc)   /* root device, kdev_t or BIOS dev */
#define SETUP_ELKS_FLAGS        setupb(0x1f6)   /* flags for root device type */
#define SETUP_PART_OFFSETLO     setupw(0x1e2)   /* partition offset low word */
#define SETUP_PART_OFFSETHI     setupw(0x1e4)   /* partition offset high word */
#ifdef CONFIG_ROMCODE
#define SYS_CAPS                (CAP_PC_AT)
#endif
#define UTS_MACHINE             "ibmpc i8086"

/* The following can be set for minimal systems or for QEMU emulation testing:
 * 10 buffers (@20 = 200), 2 ttyq (@80 = 160), 4k L1 cache, 512 heap free,
 * 10 tasks (@876 = 8760), 64 inodes (@80 = 5120), 64 files (@14 = 896) = ~19744.
 * Use buf=10 cache=4 task=10 inode=64 file=64  in /bootopts
 */
#if defined(CONFIG_HW_MK88)
#define SETUP_HEAPSIZE            19744         /* force kernel heap size */
#endif
//#undef SETUP_MEM_KBYTES
//#define SETUP_MEM_KBYTES        256     /* force available memory in 1K bytes */
//#define SETUP_MEM_KBYTES_ASM    SETUP_MEM_KBYTES

#endif /* CONFIG_ARCH_IBMPC */

#ifdef CONFIG_ARCH_PC98
#define MAX_SERIAL              1       /* max number of serial tty devices*/
#define SETUP_VID_COLS          80      /* video # columns */
#define SETUP_VID_LINES         25      /* video # lines */
#define SETUP_CPU_TYPE          1       /* processor type = 8086 */
#define SETUP_MEM_KBYTES        setupw(0x2a)    /* base memory in 1K bytes */
#define SETUP_ROOT_DEV          setupw(0x1fc)   /* root device, kdev_t or BIOS dev */
#define SETUP_ELKS_FLAGS        setupb(0x1f6)   /* flags for root device type */
#define SETUP_PART_OFFSETLO     setupw(0x1e2)   /* partition offset low word */
#define SETUP_PART_OFFSETHI     setupw(0x1e4)   /* partition offset high word */
#define SYS_CAPS                CAP_IRQ8TO15    /* enable capabilities */
#define UTS_MACHINE             "pc-98 i8086"

#define CONFIG_VAR_SECTOR_SIZE  /* sector size may vary across disks */
//#undef SETUP_MEM_KBYTES
//#define SETUP_MEM_KBYTES        640     /* force available memory in 1K bytes */
//#define SETUP_MEM_KBYTES_ASM    SETUP_MEM_KBYTES

#endif /* CONFIG_ARCH_PC98 */

#ifdef CONFIG_ARCH_8018X
#define MAX_SERIAL              2       /* max number of serial tty devices*/
#define SETUP_VID_COLS          80      /* video # columns */
#define SETUP_VID_LINES         25      /* video # lines */
#define SETUP_CPU_TYPE          5       /* processor type 80186 */
#define SETUP_MEM_KBYTES        512     /* base memory in 1K bytes */
#define SETUP_ROOT_DEV          0x0600  /* root device ROMFS */
#define SETUP_ELKS_FLAGS        0       /* flags for root device type */
#define SETUP_PART_OFFSETLO     0       /* partition offset low word */
#define SETUP_PART_OFFSETHI     0       /* partition offset high word */
#define SYS_CAPS                0       /* no XT/AT capabilities */
#define UTS_MACHINE             "8018x"

#define CONFIG_8018X_FCPU       16
#define CONFIG_8018X_EB
#endif

/*
 * System capabilities - configurable for ROM or custom installations.
 * Normally, all capabilities will be set if arch_cpu > 5 (PC/AT),
 * except when SYS_CAPS is defined for custom installations or emulations.
 */
#define CAP_PC_AT       (CAP_IRQ8TO15|CAP_IRQ2MAP9)      /* PC/AT capabilities */
#define CAP_KBD_LEDS    0x04    /* has keyboard LEDs */
#define CAP_HD_IDE      0x08    /* can do hard drive IDE probes */
#define CAP_IRQ8TO15    0x10    /* has IRQ 8 through 15 */
#define CAP_IRQ2MAP9    0x20    /* map IRQ 2 to 9 */
#define CAP_ALL         0xFF    /* all capabilities if PC/AT only */

/* Don't touch these, unless you really know what you are doing. */
#define DEF_INITSEG     0x0100  /* initial Image load address by boot code */
#define DEF_SYSSEG      0x1300  /* address setup then copies kernel to, then REL_SYSSEG */
#define DEF_SETUPSEG    DEF_INITSEG + 0x20
#define DEF_SYSMAX      0x2F00  /* maximum system size (=.text+.fartext+.data) */

/* Segmemnt DMASEG up to DMASEGEND is used as a bounce buffer of at least 1K (=BLOCKSIZE)
 * below the first 64K boundary (=0x1000:0) for use with the old 8237 DMA controller.
 * If floppy track caching is enabled, this area is also used for the track buffer
 * for direct DMA access using multisector I/O.
 * Following DMASEGEND is the kernel code and data at REL_SYSSEG.
 */

#ifdef CONFIG_TRACK_CACHE           /* floppy track buffer in low mem */
#define DMASEGSZ        0x2400      /* SECTOR_SIZE * 18 (9216) */
#else
#define DMASEGSZ        0x0400      /* BLOCK_SIZE (1024) */
#endif
#define DMASEGEND       (DMASEG+(DMASEGSZ>>4))

#ifdef CONFIG_ROMCODE
#if defined(CONFIG_BLK_DEV_BFD) || defined(CONFIG_BLK_DEV_BHD)  /* BIOS disk driver*/
#define DMASEG          0x80        /* 0x400 bytes floppy sector buffer */
#define KERNEL_DATA     DMASEGEND   /* kernel data segment */
#else
#define KERNEL_DATA     0x080       /* kernel data segment */
#endif
#define SETUP_DATA      CONFIG_ROM_SETUP_DATA
#endif

#if (defined(CONFIG_ARCH_IBMPC) || defined(CONFIG_ARCH_8018X)) && !defined(CONFIG_ROMCODE)
/* Define segment locations of low memory, must not overlap */
#define OPTSEGSZ        0x200       /* max size of /bootopts file (512 bytes max) */
#define DEF_OPTSEG      0x50        /* 0x200 bytes boot options at lowest usable ram */
#define REL_INITSEG     0x70        /* 0x200 bytes setup data */
#define DMASEG          0x90        /* start of floppy sector buffer */
#define REL_SYSSEG      DMASEGEND   /* kernel code segment */
#define SETUP_DATA      REL_INITSEG
#endif

#if defined(CONFIG_ARCH_PC98) && !defined(CONFIG_ROMCODE)
/* Define segment locations of low memory, must not overlap */
#define OPTSEGSZ        0x200       /* max size of /bootopts file (512 bytes max) */
#define DEF_OPTSEG      0x60        /* 0x200 bytes boot options at lowest usable ram */
#define REL_INITSEG     0x80        /* 0x200 bytes setup data */
#define DMASEG          0xA0        /* start of floppy sector buffer */
#define REL_SYSSEG      DMASEGEND   /* kernel code segment */
#define SETUP_DATA      REL_INITSEG
#endif

#endif
