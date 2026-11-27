#ifndef __ARCH_8086_SEGMENT_H
#define __ARCH_8086_SEGMENT_H

/*
 * Configured values for various fixed segment addresses and selectors.
 */
#include <linuxmt/config.h>

/* fixed kernel-private selectors */
#define SEL_NULL        0x00  /* required null descriptor */
#define SEL_KCODE       0x08  /* kernel CS  (near .text) */
#define SEL_KDATA       0x10  /* kernel DS = SS */
#define SEL_KFTEXT      0x18  /* kernel far text (.fartext, medium model) */
#define SEL_KDATA_EXEC  0x20  /* kernel data aliased as readable CODE (IRQ trampolines) */
#define SEL_GDT         0x28  /* GDT table access (directly follows kernel data segment */
#define SEL_IDT         0x30  /* IDT table access at physical address 0:0 */
#define SEL_SETUP       0x38  /* setup.S data segment  (SEG_INITSEG, for setupb/setupw) */
                              /* 0x40 used for SEL_BIOSDATA below */
#define SEL_OPTSEG      0x48  /* boot options area (SEG_OPTSEG, /bootopts at init) */
#define SEL_TRACKBUF    0x50  /* directfd DMA track buffer (SEG_TRACK) */
#define SEL_DMABUF      0x58  /* ATA/CF DMA sector buffer (SEG_DMASEG) */
#define SEL_ROMFONT     0x60  /* location of ROM font for VGA text mode restore */
#define SEL_FIRST_DYN   0x68  /* first dynamically-allocated selector */

/* identical segment/selectors for video text, graphics and BDA */
#define SEL_BIOSDATA    SEG_BIOSDATA   /* BIOS data area at 0040 */
#define SEL_VIDEO       SEG_VIDEO      /* CGA/EGA/VGA text video memory at B800 */
#define SEL_GRAPHICS    SEG_GRAPHICS   /* EGA/VGA graphics video memory at A000 */

#define GDT_SIZE        ((unsigned)(MAX_GDT_ENTRIES << 3))

/*
 * Protected mode selector vs real mode segment definitions and macros
 */
#ifdef CONFIG_286_PMODE

/* macros map to selector values */
#define KERNEL_CS       SEL_KCODE       /* kernel near code selector */
#define KERNEL_DS       SEL_KDATA       /* kernel data selector */
#define SETUP_DATA      SEL_SETUP       /* setupb/setupw data segment */
#define OPTSEG          SEL_OPTSEG      /* /bootopts options segment */
#define BIOSSEG         SEL_BIOSDATA    /* BIOS data */
#define VIDEOSEG        SEL_VIDEO       /* text video RAM */
#define GRAPHICSSEG     SEL_GRAPHICS    /* graphics video RAM */
#define TRACKSEG        SEL_TRACKBUF    /* direct floppy track cache */
#define DMASEG          SEL_DMABUF      /* ata/cf dma buffer */

/* PM address conversion macros used in directfd.c */
#define LINADDR(seg, offs) (desc_base(seg) + (unsigned long)(unsigned)(offs))
#define XMSADDR(seg, offs) ((unsigned long)((((unsigned long)(seg)) << 0) + (unsigned)(offs)))

#else /* real mode */

/* macros map to segment values */
#define KERNEL_CS       kernel_cs       /* real mode kernel near code segment */
#define KERNEL_DS       kernel_ds       /* real mode kernel data segment */
#define SETUP_DATA      SEG_INITSEG     /* setupb/setupw data segment */
#define OPTSEG          SEG_OPTSEG      /* /bootopts options segment */
#define BIOSSEG         SEG_BIOSDATA    /* BIOS data */
#define VIDEOSEG        SEG_VIDEO       /* text video RAM */
#define GRAPHICSSEG     SEG_GRAPHICS    /* graphics video RAM */
#define TRACKSEG        SEG_TRACK       /* direct floppy track cache */
#define DMASEG          SEG_DMASEG      /* ata/cf dma buffer */

/* real mode address conversion macros used in directfd.c */
#define LINADDR(seg, offs) ((unsigned long)((((unsigned long)(seg)) << 4) + (unsigned)(offs)))
#define XMSADDR(seg, offs) ((unsigned long)((((unsigned long)(seg)) << 0) + (unsigned)(offs)))
#endif

#endif
