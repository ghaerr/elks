#ifndef __LINUXMT_CONFIG_H
#define __LINUXMT_CONFIG_H

#include <autoconf.h>
#include <linuxmt/major.h>

/*
 * Compile-time configuration
 */
#define UTS_SYSNAME "ELKS"                      /* uname system name */
#define UTS_NODENAME "elks"                     /* someday set by sethostname() */

#define CONFIG_FS_DEV           1               /* support FAT /dev folder */

/*
 * SETUP_ defines are initialzied by setup.S and queried only during kernel init.
 * The REL_INITSEG segment is released at end of kernel init. If used later any
 * values must be copied, as afterwards setupb/setupw will return incorrect data.
 * These defines are overridden for ROM based systems w/o setup code.
 * See setup.S for setupb/setupw offsets.
 */

#ifdef CONFIG_ARCH_IBMPC
#define MAX_SERIAL              4               /* max number of serial tty devices*/
#define SETUP_VID_COLS          setupb(7)       /* BIOS video # columns */
#define SETUP_VID_LINES         setupb(14)      /* BIOS video # lines */
#define SETUP_CPU_TYPE          setupb(0x20)    /* processor type */
#define SETUP_MEM_KBYTES        setupw(0x2a)    /* base memory in 1K bytes */
#define SETUP_XMS_KBYTES        setupw(0x1ea)   /* xms memory in 1K bytes */
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
#define SETUP_CPU_TYPE          setupb(0x20)    /* processor type */
#define SETUP_MEM_KBYTES        setupw(0x2a)    /* base memory in 1K bytes */
#define SETUP_XMS_KBYTES        setupw(0x1ea)   /* xms memory in 1K bytes */
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
#define SETUP_CPU_TYPE          CPU_80186  /* processor type */
#define SETUP_MEM_KBYTES        512     /* base memory in 1K bytes */
#define SETUP_XMS_KBYTES        0       /* xms memory in 1K bytes */
#define SETUP_ROOT_DEV          0x0600  /* root device ROMFS */
#define SETUP_ELKS_FLAGS        0       /* flags for root device type */
#define SETUP_PART_OFFSETLO     0       /* partition offset low word */
#define SETUP_PART_OFFSETHI     0       /* partition offset high word */
#define SYS_CAPS                0       /* no XT/AT capabilities */
#define UTS_MACHINE             "8018x"

#define CONFIG_8018X_FCPU       16
#define CONFIG_8018X_EB
#endif

#ifdef CONFIG_ARCH_SWAN
#define MAX_SERIAL              1       /* max number of serial tty devices*/
#define SETUP_VID_COLS          28      /* video # columns */
#define SETUP_VID_LINES         18      /* video # lines */
#define SETUP_CPU_TYPE          CPU_80186  /* processor type */
#define SETUP_MEM_KBYTES        128     /* base memory in 1K bytes */
#define SETUP_XMS_KBYTES        0       /* xms memory in 1K bytes */
#define SETUP_ROOT_DEV          0x0600  /* root device ROMFS */
#define SETUP_ELKS_FLAGS        0       /* flags for root device type */
#define SETUP_PART_OFFSETLO     0       /* partition offset low word */
#define SETUP_PART_OFFSETHI     0       /* partition offset high word */
#define SYS_CAPS                0       /* no XT/AT capabilities */
#define UTS_MACHINE             "swan"
#define SETUP_HEAPSIZE          32256   /* 0x8000 - 0xFDFF */
#define SETUP_USERHEAPSEG       0x1000  /* start segment for appiication memory heap */
#endif /* CONFIG_ARCH_SWAN */

#ifdef CONFIG_ARCH_SOLO86
#define MAX_SERIAL              0       /* max number of serial tty devices*/
#define SETUP_VID_COLS          80      /* video # columns */
#define SETUP_VID_LINES         25      /* video # lines */
#define SETUP_CPU_TYPE          CPU_80286  /* processor type */
#define SETUP_MEM_KBYTES        512     /* base memory in 1K bytes */
#define SETUP_XMS_KBYTES        0       /* xms memory in 1K bytes */
#define SETUP_ROOT_DEV          0x0600  /* root device ROMFS */
#define SETUP_ELKS_FLAGS        0       /* flags for root device type */
#define SETUP_PART_OFFSETLO     0       /* partition offset low word */
#define SETUP_PART_OFFSETHI     0       /* partition offset high word */
#define SYS_CAPS                0       /* no XT/AT capabilities */
#define UTS_MACHINE             "Solo/86"
#endif /* CONFIG_ARCH_SOLO86 */

/* linear address to start XMS buffer allocations from */
#define XMS_START_ADDR    0x00100000L	/* 1M */
//#define XMS_START_ADDR  0x00FA0000L	/* 15.6M (Compaq with only 1M ram) */

/*
 * System capabilities - configurable for ROM or custom installations.
 * Normally, all capabilities will be set if arch_cpu >= CPU_80286 (PC/AT),
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
#define DEF_SYSSEG      0x1400  /* address setup then copies kernel to, then REL_SYSSEG */
#define DEF_SETUPSEG    DEF_INITSEG + 0x20
#define DEF_SYSMAX      0x2F00  /* maximum system size (=.text+.fartext+.data) */

/* Segment DMASEG up to DMASEGEND is used for the XMS/DMA bounce buffer and track cache.
 * Following DMASEGEND is the kernel code at REL_SYSSEG (or kernel data for ROM configs).
 *
 * A "bounce buffer" is configured below the first 64K boundary for use with
 * old 8237 DMA controller which wraps addresses wider than 16-bits on PC/XT systems.
 * If floppy track caching is enabled, the track buffer is also configured in
 * low memory for direct DMA access usig multisector I/O.
 * The DF driver uses the first sector of its track cache for the DMA buffer,
 * and doesn't require an XMS buffer, instead programming the 8237 external page register.
 * In contrast, the BIOS FD/HD driver is configured to use an XMS/DMA buffer
 * outside its track cache; thus the complicated defines below.
 */

#if defined(CONFIG_BLK_DEV_BFD) || defined(CONFIG_BLK_DEV_BHD)  /* BIOS driver */
#define DMASEGSZ        0x0400      /* BLOCK_SIZE (1024) for external XMS/DMA buffer */
#elif defined(CONFIG_FS_XMS) && defined(CONFIG_BLK_DEV_ATA_CF)
#define DMASEGSZ        0x200       /* ATA_SECTOR_SIZE (512) XMS buffer for ATA CF */
#else
#define DMASEGSZ        0           /* no external XMS/DMA buffer */
#endif

#if defined(CONFIG_BLK_DEV_BFD) || defined(CONFIG_BLK_DEV_BHD) || defined(CONFIG_BLK_FD)
#ifdef CONFIG_TRACK_CACHE           /* floppy track buffer in low mem */
#   define TRACKSEGSZ   0x2400      /* SECTOR_SIZE * 18 (9216) */
#else
#  ifdef CONFIG_BLK_FD
#  define TRACKSEGSZ    0x0400      /* DF driver requires DMASEG internal to TRACKSEG */
#  else
#  define TRACKSEGSZ    0           /* no TRACKSEG buffer */
#  endif
#endif
#define TRACKSEG        (DMASEG+(DMASEGSZ>>4))
#define DMASEGEND       (DMASEG+(DMASEGSZ>>4)+(TRACKSEGSZ>>4))
#else
#define DMASEGEND       (DMASEG+(DMASEGSZ>>4))
#endif

/* Define segment locations of low memory, must not overlap */

#ifdef CONFIG_ROMCODE
#define DMASEG          0x80        /* start of floppy sector buffer */
#define KERNEL_DATA     DMASEGEND   /* kernel data segment */
#define SETUP_DATA      CONFIG_ROM_SETUP_DATA

#else /* !CONFIG_ROMCODE */

#ifdef CONFIG_ARCH_IBMPC
#define OPTSEGSZ        0x400       /* max size of /bootopts file (1024 bytes max) */
#define DEF_OPTSEG      0x50        /* 0x400 bytes boot options at lowest usable ram */
#define LOADALL_SEG     0x80        /* LOADALL buffer from 0x800-0x865 on 80286 CPU */
#define REL_INITSEG     0x90        /* 0x200 bytes setup data */
#define DMASEG          0xB0        /* start of floppy sector buffer */
#define REL_SYSSEG      DMASEGEND   /* kernel code segment */
#define SETUP_DATA      REL_INITSEG
#endif

#ifdef CONFIG_ARCH_PC98
#define OPTSEGSZ        0x400       /* max size of /bootopts file (1024 bytes max) */
#define DEF_OPTSEG      0x60        /* 0x400 bytes boot options at lowest usable ram */
#define LOADALL_SEG     0x80        /* LOADALL buffer from 0x800-0x865 on 80286 CPU */
#define REL_INITSEG     0xA0        /* 0x200 bytes setup data */
#define DMASEG          0xC0        /* start of floppy sector buffer */
#define REL_SYSSEG      DMASEGEND   /* kernel code segment */
#define SETUP_DATA      REL_INITSEG
#endif

#endif /* !CONFIG_ROMCODE */

#endif
