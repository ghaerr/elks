#ifndef __ARCH_8086_SEGMENT_H
#define __ARCH_8086_SEGMENT_H

#include <linuxmt/config.h>

/* paragraph (16-byte) helpers shared with the real-mode arena allocator */
#define PARA_BYTES(paras)   ((addr_t)(paras) << 4)
#define BYTES_PARA(bytes)   (((bytes) + 15) >> 4)

/*
 * Protected mode selector vs segment definitions and macros
 */
#ifdef CONFIG_286_PMODE

/* fixed GDT indices (kernel-private selectors) */
#define GDT_NULL        0   /* required null descriptor          */
#define GDT_KCODE       1   /* kernel CS  (near .text)           */
#define GDT_KDATA       2   /* kernel DS = SS                    */
#define GDT_KFTEXT      3   /* kernel far text (.fartext, medium model) */
#define GDT_KDATA_EXEC  4   /* kernel data aliased as readable CODE (IRQ trampolines) */
#define GDT_SETUP       5   /* boot setup-data (REL_INITSEG, used by setupb/setupw) */
#define GDT_OPTSEG      6   /* boot options area (DEF_OPTSEG, /bootopts at init) */
#define GDT_BIOSDATA    7   /* BIOS data area (segment 0x40) */
#define GDT_VIDEO       8   /* CGA/EGA/VGA text video memory (segment 0xB800) */
#define GDT_TRACK       9   /* directfd DMA/track buffer (TRACKSEG) */
#define GDT_FIRST_DYN   10  /* first dynamically-allocated selector */

/* fixed kernel selectors (index<<3, GDT, RPL0) -- must match above order */
#define SEL_KCODE       0x08
#define SEL_KDATA       0x10
#define SEL_KFTEXT      0x18
#define SEL_KDATA_EXEC  0x20
#define SEL_SETUP       0x28
#define SEL_OPTSEG      0x30
#define SEL_BIOSDATA    0x38
#define SEL_VIDEO       0x40
#define SEL_TRACK       0x48

#define GDT_ENTRIES     512 /* 512 * 8 = 4 KB GDT                 */

/* macros map to selector values */
#define KERNEL_CS       SEL_KCODE       /* kernel near code selector */
#define KERNEL_DS       SEL_KDATA       /* kernel data selector */
#define BIOSSEG         SEL_BIOSDATA    /* BIOS data */
#define VIDEOSEG        SEL_VIDEO       /* text video RAM */
#define TRACKSEG        SEL_TRACK       /* direct floppy track cache */
#define SETUP_DATA      SEL_SETUP       /* setupb/setupw data segment */

/* PM address conversion macros used in directfd.c */
#define LINADDR(seg, offs) (desc_base(seg) + (unsigned long)(unsigned)(offs))
#define XMSADDR(seg, offs) ((unsigned long)((((unsigned long)(seg)) << 0) + (unsigned)(offs)))

#else /* real mode */

/* macros map to segment values */
#define KERNEL_CS       kernel_cs       /* real mode kernel near code segment */
#define KERNEL_DS       kernel_ds       /* real mode kernel data segment */
#define BIOSSEG         SEG_BIOSDATA    /* BIOS data */
#define VIDEOSEG        SEG_VIDEO       /* text video RAM */
#define TRACKSEG        SEG_TRACK       /* direct floppy track cache */
#define SETUP_DATA      SEG_SETUP_DATA  /* setupb/setupw data segment */

/* real mode address conversion macros used in directfd.c */
#define LINADDR(seg, offs) ((unsigned long)((((unsigned long)(seg)) << 4) + (unsigned)(offs)))
#define XMSADDR(seg, offs) ((unsigned long)((((unsigned long)(seg)) << 0) + (unsigned)(offs)))
#endif

#ifndef __ASSEMBLER__
#include <linuxmt/types.h>

extern seg_t kernel_cs, kernel_ds;
extern short *_endtext, *_endftext, *_enddata, *_endbss;
extern short endistack[], istack[];
extern unsigned int heapsize;
#endif

#endif
