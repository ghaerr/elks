#ifndef __ARCH_8086_SEGMENT_H
#define __ARCH_8086_SEGMENT_H

#include <linuxmt/config.h>

/*
 * Protected mode selector vs segment macros
 */
#ifdef CONFIG_286_PMODE
/* (fixed) protected mode selector values defined in seg286.h */
//#include <arch/seg286.h>      // add this and remove SEL_ below when arch/seg286.h present
#define SEL_KCODE       0x08
#define SEL_KDATA       0x10
#define SEL_KFTEXT      0x18
#define SEL_SETUP       0x28    /* GDT[5]: base = REL_INITSEG<<4, for setupb/setupw in PM */
#define SEL_OPTSEG      0x30    /* GDT[6]: base = DEF_OPTSEG<<4, for the /bootopts copy in PM */
#define SEL_BIOSDATA    0x38    /* GDT[7]: base = 0x400, BIOS data area (seg 0x40) reads in PM */
#define SEL_VIDEO       0x40    /* GDT[8]: base = 0xB8000, text video memory (VideoSeg) in PM */
#define SEL_TRACK       0x48    /* GDT[9]: base = TRACKSEG<<4, directfd DMA/track buffer in PM */
#define SEL_KDATA_EXEC  0x20    /* GDT[4]: same base as KDATA, but executable+readable;
                                 * IDT gates point here so the CPU can run the trampolines
                                 * that int_handler_add() builds in the kernel data segment */

/* macros map to selector values */
#define KERNEL_CS       SEL_KCODE       /* kernel near code selector */
#define KERNEL_DS       SEL_KDATA       /* kernel data selector */
#define BIOSSEG         SEL_BIOSDATA    /* BIOS data */
#define VIDEOSEG        SEL_VIDEO       /* text video RAM */
#define TRACKSEG        SEL_TRACK       /* direct floppy track cache */
#define SETUP_DATA      SEL_SETUP       /* setupb/setupw data segment */

/* address conversion macros used in directfd.c */
#define LINADDR(seg, offs) (desc_base(seg) + (unsigned long)(unsigned)(offs))
#define XMSADDR(seg, offs) ((unsigned long)((((unsigned long)(seg)) << 0) + (unsigned)(offs)))

#else /* real mode */

/* use real mode segment values (these may move to config.h) */
#define SEG_BIOSDATA    0x0040          /* BIOS data */
#define SEG_VIDEO       0xB800          /* text video RAM */

/* macros map to segment values */
#define KERNEL_CS       kernel_cs       /* real mode kernel near code segment */
#define KERNEL_DS       kernel_ds       /* real mode kernel data segment */
#define BIOSSEG         SEG_BIOSDATA    /* BIOS data */
#define VIDEOSEG        SEG_VIDEO       /* text video RAM */
#define TRACKSEG        SEG_TRACK       /* direct floppy track cache */
#define SETUP_DATA      SEG_SETUP_DATA  /* setupb/setupw data segment */

/* address conversion macros used in directfd.c */
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
