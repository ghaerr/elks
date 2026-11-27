/*
 * ELKS Graphics VGA hardware initialization and return to text mode
 *
 * Copyright (c) 1999, 2026 Greg Haerr <greg@censoft.com>
 * Copyright (c) 1991 David I. Bell
 */

#include <stdlib.h>
#include <string.h>
#include <linuxmt/segment.h>
#include <arch/segment.h>
#include <arch/io.h>

int rom_char_height;
int rom_font_number;

/* Define hardware init default to be VGA or EGA */
#define VGA_STANDARD    1       /* standard VGA 640x480*/
#define EGA_STANDARD    0       /* standard EGA 640x350*/

#define DONE    0
#define IN      1
#define OUT     2

#define PALREG  0x3c0
#define SEQREG  0x3c4
#define SEQVAL  0x3c5
#define GRREG   0x3ce
#define GRVAL   0x3cf
#define ATTRREG 0x3da
#define CRTCREG 0x3d4
#define CRTCVAL 0x3d5
#define GENREG1 0x3c2
#define GENREG2 0x3cc
#define GENREG3 0x3ca

#define out_byte(port, val)     outb(val, port)
#define out_word(port, val)     outw(val, port)

typedef struct {
    int action;
    int port1;
    int data1;
    int port2;
    int data2;
} REGIO;

/* local data*/
static REGIO    graphics_on[];
static REGIO    graph_off[];

/* local routines*/
static void writeregs(REGIO *rp);

#if 0
/* return true if running on PM kernel */
#define runningPM() __extension__ ({                \
    unsigned int _v;                                \
    asm volatile (                                  \
        ".byte 0x0f, 0x01, 0xe0\n"  /* smsw %ax */  \
        "and $1,%%ax\n"                             \
        :"=a" (_v)                                  \
        :                                           \
        : "cc"                                      \
    );                                              \
    _v; })
#endif

/* return true if SMSW safe to execute (requires 80286+) */
static int has_smsw(void)
{
    unsigned flags_after;

    __asm__ __volatile__ (
        "pushf\n\t"                 /* Save original FLAGS */
        "pushf\n\t"                 /* Push another copy to modify */
        "pop %%ax\n\t"              /* Load FLAGS into AX */
        "and $0x0fff, %%ax\n\t"     /* Clear bits 12-15 */
        "push %%ax\n\t"             /* Push modified value onto stack */
        "popf\n\t"                  /* Pop into FLAGS register */
        "pushf\n\t"                 /* Push resulting FLAGS back */
        "pop %0\n\t"                /* Read resulting FLAGS into flags_after */
        "popf"                      /* Restore original FLAGS */
        : "=r" (flags_after)
        :
        : "ax", "cc"
    );

    /* If bits 12-15 are all still 1 (0xF000), it is an 8086/8088 or 80186/80188 */
    return (flags_after & 0xF000) != 0xF000;
}

/* return true if running on ELKS PM kernel */
static int runningPM(void)
{
    unsigned int pm;

    if (!has_smsw())
        return 0;                   /* CPU is 8086/80186 */

    __asm__ __volatile__ (
        ".byte 0x0f, 0x01, 0xe0\n\t"/* smsw %ax (can assemble with -march=i8086) */
        "and $1,%%ax\n"
        : "=a" (pm)
    );
    return pm;
}

/* Query BIOS INT 10h / AX=1130h for the ROM font address */
static unsigned char __far *get_rom_font(int font_type)
{
    unsigned int seg, off;
    __asm__ __volatile__ (
        "pushw %%bp\n\t"          /* INT 10h/1130h clobbers BP with the offset */
        "int $0x10\n\t"
        "movw %%es, %0\n\t"
        "movw %%bp, %1\n\t"
        "popw %%bp\n\t"
        : "=r"(seg), "=r"(off)
        : "a"(0x1130), "b"(font_type << 8)
        : "cx", "dx", "cc"
    );
    return _MK_FP(seg, off);
}

static void load_vga_font(unsigned char __far *font_src, int char_height, int slot)
{
    /* Plane 2 font slots are spaced every 8 KB (0x2000) in the A0000h aperture */
    static const unsigned int slot_offsets[8] = {
        0x0000, 0x4000, 0x8000, 0xC000, 0x2000, 0x6000, 0xA000, 0xE000
    };

    unsigned char __far *vga_plane2 = _MK_FP(0xA000, slot_offsets[slot & 7]);

    /* 1. Configure Sequencer (0x3C4) & Graphics Controller (0x3CE) for Plane 2 access */
out_word(SEQREG, 0x0100);       /* syn reset */
    out_word(SEQREG, 0x0402);   /* Map Mask (2): enable writes to Plane 2 only (bit 2) */
    //out_word(SEQREG, 0x0604); /* Memory Mode (4): disable Odd/Even (bit 2), enable Ext Mem (bit 1) */
out_word(SEQREG, 0x0704);       /* Memory Mode (4): disable Odd/Even (bit 2), enable Ext Mem (bit 1) */
out_word(SEQREG, 0x0300);       /* clear synchronous reset */

out_word(GRREG, 0x0001);
out_word(GRREG, 0x0003);        /* drawing mode to COPY/NOP */
out_word(GRREG, 0x0204);
    out_word(GRREG, 0x0005);    /* Graphics Mode (5): Write Mode 0, disable Odd/Even (bit 4) */
    out_word(GRREG, 0x0406);    /* Misc Graphics (6): map VRAM to A0000h-AFFFFh (bits 3:2 = 01b) */
out_word(GRREG, 0xFF08);

    //__dprintf("rom %lx height %d\n", font_src, char_height);
    /* 2. Copy glyphs: VGA hardware ALWAYS reserves 32 bytes per character in Plane 2,
     * regardless of actual character height (14 or 16 rows).
     */
    for (int ch = 0; ch < 256; ch++) {
        int dst_base = ch * 32;
        int src_base = ch * char_height;
        int row;
        for (row = 0; row < char_height; row++) {
            vga_plane2[dst_base + row] = font_src[src_base + row];
        }
        //for ( ; row < 32; row++)
            //vga_plane2[dst_base + row] = 0;
    }

    /* 3. Restore standard VGA text mode (Odd/Even addressing at B8000h) */
    out_word(SEQREG, 0x0302);   /* Map Mask (2): enable Planes 0 & 1 (chars & attributes) */
    out_word(SEQREG, 0x0204);   /* Memory Mode (4): enable Odd/Even, Ext Mem */
//out_word(SEQREG, 0x0304);     /* Memory Mode (4): enable Odd/Even, Ext Mem */
out_word(GRREG, 0x0004);        /* Read Map Select: Plane 0 */
    out_word(GRREG, 0x1005);    /* Graphics Mode (5): enable Odd/Even (bit 4) */
    out_word(GRREG, 0x0E06);    /* Misc Graphics (6): map VRAM to B8000h-BFFFFh, Odd/Even on */
}

void
vga_hwinit(void)
{
    writeregs(graphics_on);
    fmemset(_MK_FP(0xA000, 0), 0, 0x8000);
    fmemset(_MK_FP(0xA000, 0x8000), 0, 0x8000);
}

void
vga_hwterm(void)
{
    unsigned char __far *rom_font_address;
    //__dprintf("RUN %d\n", runningPM());

    if (runningPM()) {
        /* selector-based address of ROM font is passed in BDA:00F0 by PM kernel */
        unsigned long __far *bda = _MK_FP(SEL_BIOSDATA, 0x00F0);
        //__dprintf("BDA %08lx\n", *bda);
        rom_font_address = (unsigned char __far *)*bda;
    } else {
        rom_font_address = get_rom_font(rom_font_number);
    }
    //__dprintf("ROM %08lx\n", rom_font_address);
    
    load_vga_font(rom_font_address, rom_char_height, 0);

    /* back to text mode */
    //restore_dac_palette();
    writeregs(graph_off);

    fmemset(_MK_FP(0xB800, 0), 0, 80*25*2);
}

/* Set the graphics registers as indicated by the given table */
static void
writeregs(REGIO *rp)
{
    for (; rp->action != DONE; rp++) {
        switch (rp->action) {
        case IN:
            inb(rp->port1);
            break;
        case OUT:
            out_byte(rp->port1, rp->data1);
            if (rp->port2)
                out_byte(rp->port2, rp->data2);
            break;
        }
    }
}

#pragma GCC diagnostic ignored "-Wmissing-braces"

#if VGA_STANDARD

int rom_char_height = 16;
int rom_font_number = 6;

/* VGA 640x480 16-color graphics (BIOS mode 0x12).
 */
static REGIO graphics_on[] = {
  /* Reset attr F/F */
  IN, ATTRREG, 0, 0, 0,

  /* Disable palette */
  OUT, PALREG, 0, 0, 0,

  /* Reset sequencer regs */
  OUT, SEQREG, 0, SEQVAL, 0,
  OUT, SEQREG, 1, SEQVAL, 1,
  OUT, SEQREG, 2, SEQVAL, 0x0f,
  OUT, SEQREG, 3, SEQVAL, 0,
  OUT, SEQREG, 4, SEQVAL, 6,

  /* Misc out reg */
  OUT, GENREG1, 0xe3, 0, 0,

  /* Sequencer enable */
  OUT, SEQREG, 0, SEQVAL, 0x03,

  /* Unprotect crtc regs 0-7 */
  OUT, CRTCREG, 0x11, CRTCVAL, 0,

  /* Crtc */
  OUT, CRTCREG, 0, CRTCVAL, 0x5f,
  OUT, CRTCREG, 1, CRTCVAL, 0x4f,
  OUT, CRTCREG, 2, CRTCVAL, 0x50,
  OUT, CRTCREG, 3, CRTCVAL, 0x82,
  OUT, CRTCREG, 4, CRTCVAL, 0x54,
  OUT, CRTCREG, 5, CRTCVAL, 0x80,
  OUT, CRTCREG, 6, CRTCVAL, 0x0b,
  OUT, CRTCREG, 7, CRTCVAL, 0x3e,
  OUT, CRTCREG, 8, CRTCVAL, 0x00,
  OUT, CRTCREG, 9, CRTCVAL, 0x40,
  OUT, CRTCREG, 10, CRTCVAL, 0x00,
  OUT, CRTCREG, 11, CRTCVAL, 0x00,
  OUT, CRTCREG, 12, CRTCVAL, 0x00,
  OUT, CRTCREG, 13, CRTCVAL, 0x00,
  OUT, CRTCREG, 14, CRTCVAL, 0x00,
  OUT, CRTCREG, 15, CRTCVAL, 0x59,
  OUT, CRTCREG, 16, CRTCVAL, 0xea,
  OUT, CRTCREG, 17, CRTCVAL, 0x8c,
  OUT, CRTCREG, 18, CRTCVAL, 0xdf,
  OUT, CRTCREG, 19, CRTCVAL, 0x28,
  OUT, CRTCREG, 20, CRTCVAL, 0x00,
  OUT, CRTCREG, 21, CRTCVAL, 0xe7,
  OUT, CRTCREG, 22, CRTCVAL, 0x04,
  OUT, CRTCREG, 23, CRTCVAL, 0xe3,
  OUT, CRTCREG, 24, CRTCVAL, 0xff,

  /* Graphics controller */
  OUT, GENREG2, 0x00, 0, 0,
  OUT, GENREG3, 0x01, 0, 0,
  OUT, GRREG, 0, GRVAL, 0x00,
  OUT, GRREG, 1, GRVAL, 0x00,
  OUT, GRREG, 2, GRVAL, 0x00,
  OUT, GRREG, 3, GRVAL, 0x00,
  OUT, GRREG, 4, GRVAL, 0x00,
  OUT, GRREG, 5, GRVAL, 0x00,
  OUT, GRREG, 6, GRVAL, 0x05,
  OUT, GRREG, 7, GRVAL, 0x0f,
  OUT, GRREG, 8, GRVAL, 0xff,

  /* Reset attribute flip/flop */
  IN, ATTRREG, 0, 0, 0,

  /* Palette */
  OUT, PALREG, 0, PALREG, 0x00,
  OUT, PALREG, 1, PALREG, 0x01,
  OUT, PALREG, 2, PALREG, 0x02,
  OUT, PALREG, 3, PALREG, 0x03,
  OUT, PALREG, 4, PALREG, 0x04,
  OUT, PALREG, 5, PALREG, 0x05,
  OUT, PALREG, 6, PALREG, 0x06,
  OUT, PALREG, 7, PALREG, 0x07,
  OUT, PALREG, 8, PALREG, 0x38,
  OUT, PALREG, 9, PALREG, 0x39,
  OUT, PALREG, 10, PALREG, 0x3a,
  OUT, PALREG, 11, PALREG, 0x3b,
  OUT, PALREG, 12, PALREG, 0x3c,
  OUT, PALREG, 13, PALREG, 0x3d,
  OUT, PALREG, 14, PALREG, 0x3e,
  OUT, PALREG, 15, PALREG, 0x3f,
  OUT, PALREG, 16, PALREG, 0x01,
  OUT, PALREG, 17, PALREG, 0x00,
  OUT, PALREG, 18, PALREG, 0x0f,
  OUT, PALREG, 19, PALREG, 0x00,

  /* Enable palette */
  OUT, PALREG, 0x20, 0, 0,

  /* End of table */
  DONE, 0, 0, 0, 0
};


/* VGA 80x25 text (BIOS mode 3).
 */
static REGIO graph_off[] = {
  /* Reset attr F/F */
  IN, ATTRREG, 0, 0, 0,

  /* Disable palette */
  OUT, PALREG, 0, 0, 0,

  /* Reset sequencer regs */
  OUT, SEQREG, 0, SEQVAL, 1,
  OUT, SEQREG, 1, SEQVAL, 1,
  OUT, SEQREG, 2, SEQVAL, 3,
  OUT, SEQREG, 3, SEQVAL, 0,
  OUT, SEQREG, 4, SEQVAL, 2,

  /* Misc out reg */
  OUT, GENREG1, 0x63, 0, 0,

  /* Sequencer enable */
  OUT, SEQREG, 0, SEQVAL, 3,

  /* Unprotect crtc regs 0-7 */
  OUT, CRTCREG, 0x11, CRTCVAL, 0,

  /* Crtc */
  OUT, CRTCREG, 0, CRTCVAL, 0x5f,   /* horiz total */
  OUT, CRTCREG, 1, CRTCVAL, 0x4f,   /* horiz end */
  OUT, CRTCREG, 2, CRTCVAL, 0x50,   /* horiz blank */
  OUT, CRTCREG, 3, CRTCVAL, 0x82,   /* end blank */
  OUT, CRTCREG, 4, CRTCVAL, 0x55,   /* horiz retrace */
  OUT, CRTCREG, 5, CRTCVAL, 0x81,   /* end retrace */
  OUT, CRTCREG, 6, CRTCVAL, 0xbf,   /* vert total */
  OUT, CRTCREG, 7, CRTCVAL, 0x1f,   /* overflows */
  OUT, CRTCREG, 8, CRTCVAL, 0x00,   /* row scan */
  OUT, CRTCREG, 9, CRTCVAL, 0x4f,   /* max scan line */
  OUT, CRTCREG, 10, CRTCVAL, 0x0d,  /* cursor start */
  OUT, CRTCREG, 11, CRTCVAL, 0x0e,  /* cursor end */

  //OUT, CRTCREG, 12, CRTCVAL, 0x0e,  /* start high addr */
  //OUT, CRTCREG, 13, CRTCVAL, 0xb0,  /* low addr */
  //OUT, CRTCREG, 14, CRTCVAL, 0x16,  /* cursor high */
  //OUT, CRTCREG, 15, CRTCVAL, 0x30,  /* cursor low */

  OUT, CRTCREG, 12, CRTCVAL, 0x00,  /* start high addr */
  OUT, CRTCREG, 13, CRTCVAL, 0x00,  /* low addr */
  OUT, CRTCREG, 14, CRTCVAL, 0x00,  /* cursor high */
  OUT, CRTCREG, 15, CRTCVAL, 0x00,  /* cursor low */

  OUT, CRTCREG, 16, CRTCVAL, 0x9c,  /* vert retrace */
  OUT, CRTCREG, 17, CRTCVAL, 0x8e,  /* retrace end */
  OUT, CRTCREG, 18, CRTCVAL, 0x8f,  /* vert end */
  OUT, CRTCREG, 19, CRTCVAL, 0x28,  /* offset */
  OUT, CRTCREG, 20, CRTCVAL, 0x1f,  /* underline */
  OUT, CRTCREG, 21, CRTCVAL, 0x96,  /* vert blank */
  OUT, CRTCREG, 22, CRTCVAL, 0xb9,  /* end blank */
  OUT, CRTCREG, 23, CRTCVAL, 0xa3,  /* crt mode */
  OUT, CRTCREG, 24, CRTCVAL, 0xff,  /* line compare */

  /* Graphics controller */
  OUT, GENREG2, 0x00, 0, 0,
  OUT, GENREG3, 0x01, 0, 0,
  OUT, GRREG, 0, GRVAL, 0x00,
  OUT, GRREG, 1, GRVAL, 0x00,
  OUT, GRREG, 2, GRVAL, 0x00,
  OUT, GRREG, 3, GRVAL, 0x00,
  OUT, GRREG, 4, GRVAL, 0x00,
  OUT, GRREG, 5, GRVAL, 0x10,
  OUT, GRREG, 6, GRVAL, 0x0e,
  OUT, GRREG, 7, GRVAL, 0x00,
  OUT, GRREG, 8, GRVAL, 0xff,

  /* Reset attribute flip/flop */
  IN, ATTRREG, 0, 0, 0,

  /* Palette */
  OUT, PALREG, 0, PALREG, 0x00,
  OUT, PALREG, 1, PALREG, 0x01,
  OUT, PALREG, 2, PALREG, 0x02,
  OUT, PALREG, 3, PALREG, 0x03,
  OUT, PALREG, 4, PALREG, 0x04,
  OUT, PALREG, 5, PALREG, 0x05,
  OUT, PALREG, 6, PALREG, 0x06,
  OUT, PALREG, 7, PALREG, 0x07,
  OUT, PALREG, 8, PALREG, 0x10,
  OUT, PALREG, 9, PALREG, 0x11,
  OUT, PALREG, 10, PALREG, 0x12,
  OUT, PALREG, 11, PALREG, 0x13,
  OUT, PALREG, 12, PALREG, 0x14,
  OUT, PALREG, 13, PALREG, 0x15,
  OUT, PALREG, 14, PALREG, 0x16,
  OUT, PALREG, 15, PALREG, 0x17,
  OUT, PALREG, 16, PALREG, 0x08,
  OUT, PALREG, 17, PALREG, 0x00,
  OUT, PALREG, 18, PALREG, 0x0f,
  OUT, PALREG, 19, PALREG, 0x00,

  /* Enable palette */
  OUT, PALREG, 0x20, 0, 0,

  /* End of table */
  DONE, 0, 0, 0, 0
};
#endif /* VGA_STANDARD */


#if EGA_STANDARD

int rom_char_height = 14;
int rom_font_number = 2;

/* EGA 640x350 16-color graphics (BIOS mode 0x10).
 */
static REGIO graphics_on[] = {
  /* Reset attr F/F */
  IN, ATTRREG, 0, 0, 0,

  /* Disable palette */
  OUT, PALREG, 0, 0, 0,

  /* Reset sequencer regs */
  OUT, SEQREG, 0, SEQVAL, 0,
  OUT, SEQREG, 1, SEQVAL, 1,
  OUT, SEQREG, 2, SEQVAL, 0x0f,
  OUT, SEQREG, 3, SEQVAL, 0,
  OUT, SEQREG, 4, SEQVAL, 6,

  /* Misc out reg */
  OUT, GENREG1, 0xa7, 0, 0,

  /* Sequencer enable */
  OUT, SEQREG, 0, SEQVAL, 0x03,

  /* Unprotect crtc regs 0-7 */
  OUT, CRTCREG, 0x11, CRTCVAL, 0,

  /* Crtc */
  OUT, CRTCREG, 0, CRTCVAL, 0x5b,
  OUT, CRTCREG, 1, CRTCVAL, 0x4f,
  OUT, CRTCREG, 2, CRTCVAL, 0x53,
  OUT, CRTCREG, 3, CRTCVAL, 0x37,
  OUT, CRTCREG, 4, CRTCVAL, 0x52,
  OUT, CRTCREG, 5, CRTCVAL, 0x00,
  OUT, CRTCREG, 6, CRTCVAL, 0x6c,
  OUT, CRTCREG, 7, CRTCVAL, 0x1f,
  OUT, CRTCREG, 8, CRTCVAL, 0x00,
  OUT, CRTCREG, 9, CRTCVAL, 0x00,
  OUT, CRTCREG, 10, CRTCVAL, 0x00,
  OUT, CRTCREG, 11, CRTCVAL, 0x00,
  OUT, CRTCREG, 12, CRTCVAL, 0x00,
  OUT, CRTCREG, 13, CRTCVAL, 0x00,
  OUT, CRTCREG, 14, CRTCVAL, 0x00,
  OUT, CRTCREG, 15, CRTCVAL, 0x00,
  OUT, CRTCREG, 16, CRTCVAL, 0x5e,
  OUT, CRTCREG, 17, CRTCVAL, 0x2b,
  OUT, CRTCREG, 18, CRTCVAL, 0x5d,
  OUT, CRTCREG, 19, CRTCVAL, 0x28,
  OUT, CRTCREG, 20, CRTCVAL, 0x0f,
  OUT, CRTCREG, 21, CRTCVAL, 0x5f,
  OUT, CRTCREG, 22, CRTCVAL, 0x0a,
  OUT, CRTCREG, 23, CRTCVAL, 0xe3,
  OUT, CRTCREG, 24, CRTCVAL, 0xff,

  /* Graphics controller */
  OUT, GENREG2, 0x00, 0, 0,
  OUT, GENREG3, 0x01, 0, 0,
  OUT, GRREG, 0, GRVAL, 0x00,
  OUT, GRREG, 1, GRVAL, 0x00,
  OUT, GRREG, 2, GRVAL, 0x00,
  OUT, GRREG, 3, GRVAL, 0x00,
  OUT, GRREG, 4, GRVAL, 0x00,
  OUT, GRREG, 5, GRVAL, 0x00,
  OUT, GRREG, 6, GRVAL, 0x05,
  OUT, GRREG, 7, GRVAL, 0x0f,
  OUT, GRREG, 8, GRVAL, 0xff,

  /* Reset attribute flip/flop */
  IN, ATTRREG, 0, 0, 0,

  /* Palette */
  OUT, PALREG, 0, PALREG, 0x00,
  OUT, PALREG, 1, PALREG, 0x01,
  OUT, PALREG, 2, PALREG, 0x02,
  OUT, PALREG, 3, PALREG, 0x03,
  OUT, PALREG, 4, PALREG, 0x04,
  OUT, PALREG, 5, PALREG, 0x05,
  OUT, PALREG, 6, PALREG, 0x06,
  OUT, PALREG, 7, PALREG, 0x07,
  OUT, PALREG, 8, PALREG, 0x38,
  OUT, PALREG, 9, PALREG, 0x39,
  OUT, PALREG, 10, PALREG, 0x3a,
  OUT, PALREG, 11, PALREG, 0x3b,
  OUT, PALREG, 12, PALREG, 0x3c,
  OUT, PALREG, 13, PALREG, 0x3d,
  OUT, PALREG, 14, PALREG, 0x3e,
  OUT, PALREG, 15, PALREG, 0x3f,
  OUT, PALREG, 16, PALREG, 0x01,
  OUT, PALREG, 17, PALREG, 0x00,
  OUT, PALREG, 18, PALREG, 0x0f,
  OUT, PALREG, 19, PALREG, 0x00,

  /* Enable palette */
  OUT, PALREG, 0x20, 0, 0,

  /* End of table */
  DONE, 0, 0, 0, 0
};


/* EGA 80x25 text (BIOS mode 3).
 */
static REGIO graph_off[] = {
  /* Reset attr F/F */
  IN, ATTRREG, 0, 0, 0,

  /* Disable palette */
  OUT, PALREG, 0, 0, 0,

  /* Reset sequencer regs */
  OUT, SEQREG, 0, SEQVAL, 1,
  OUT, SEQREG, 1, SEQVAL, 1,
  OUT, SEQREG, 2, SEQVAL, 3,
  OUT, SEQREG, 3, SEQVAL, 0,
  OUT, SEQREG, 4, SEQVAL, 3,

  /* Misc out reg */
  OUT, GENREG1, 0xa7, 0, 0,

  /* Sequencer enable */
  OUT, SEQREG, 0, SEQVAL, 3,

  /* Crtc */
  OUT, CRTCREG, 0, CRTCVAL, 0x5b,   /* horiz total */
  OUT, CRTCREG, 1, CRTCVAL, 0x4f,   /* horiz end */
  OUT, CRTCREG, 2, CRTCVAL, 0x53,   /* horiz blank */
  OUT, CRTCREG, 3, CRTCVAL, 0x37,   /* end blank */
  OUT, CRTCREG, 4, CRTCVAL, 0x51,   /* horiz retrace */
  OUT, CRTCREG, 5, CRTCVAL, 0x5b,   /* end retrace */
  OUT, CRTCREG, 6, CRTCVAL, 0x6c,   /* vert total */
  OUT, CRTCREG, 7, CRTCVAL, 0x1f,   /* overflows */
  OUT, CRTCREG, 8, CRTCVAL, 0x00,   /* row scan */
  OUT, CRTCREG, 9, CRTCVAL, 0x0d,   /* max scan line */
  OUT, CRTCREG, 10, CRTCVAL, 0x0d,  /* cursor start */
  OUT, CRTCREG, 11, CRTCVAL, 0x0e,  /* cursor end */
  OUT, CRTCREG, 12, CRTCVAL, 0x00,  /* start high addr */
  OUT, CRTCREG, 13, CRTCVAL, 0x00,  /* low addr */
  OUT, CRTCREG, 14, CRTCVAL, 0x00,  /* cursor high */
  OUT, CRTCREG, 15, CRTCVAL, 0x00,  /* cursor low */
  OUT, CRTCREG, 16, CRTCVAL, 0x5e,  /* vert retrace */
  OUT, CRTCREG, 17, CRTCVAL, 0x2b,  /* retrace end */
  OUT, CRTCREG, 18, CRTCVAL, 0x5d,  /* vert end */
  OUT, CRTCREG, 19, CRTCVAL, 0x28,  /* offset */
  OUT, CRTCREG, 20, CRTCVAL, 0x0f,  /* underline */
  OUT, CRTCREG, 21, CRTCVAL, 0x5e,  /* vert blank */
  OUT, CRTCREG, 22, CRTCVAL, 0x0a,  /* end blank */
  OUT, CRTCREG, 23, CRTCVAL, 0xa3,  /* crt mode */
  OUT, CRTCREG, 24, CRTCVAL, 0xff,  /* line compare */

  /* Graphics controller */
  OUT, GENREG2, 0x00, 0, 0,
  OUT, GENREG3, 0x01, 0, 0,
  OUT, GRREG, 0, GRVAL, 0x00,
  OUT, GRREG, 1, GRVAL, 0x00,
  OUT, GRREG, 2, GRVAL, 0x00,
  OUT, GRREG, 3, GRVAL, 0x00,
  OUT, GRREG, 4, GRVAL, 0x00,
  OUT, GRREG, 5, GRVAL, 0x10,
  OUT, GRREG, 6, GRVAL, 0x0e,
  OUT, GRREG, 7, GRVAL, 0x00,
  OUT, GRREG, 8, GRVAL, 0xff,

  /* Reset attribute flip/flop */
  IN, ATTRREG, 0, 0, 0,

  /* Palette */
  OUT, PALREG, 0, PALREG, 0x00,
  OUT, PALREG, 1, PALREG, 0x01,
  OUT, PALREG, 2, PALREG, 0x02,
  OUT, PALREG, 3, PALREG, 0x03,
  OUT, PALREG, 4, PALREG, 0x04,
  OUT, PALREG, 5, PALREG, 0x05,
  OUT, PALREG, 6, PALREG, 0x14,
  OUT, PALREG, 7, PALREG, 0x07,
  OUT, PALREG, 8, PALREG, 0x38,
  OUT, PALREG, 9, PALREG, 0x39,
  OUT, PALREG, 10, PALREG, 0x3a,
  OUT, PALREG, 11, PALREG, 0x3b,
  OUT, PALREG, 12, PALREG, 0x3c,
  OUT, PALREG, 13, PALREG, 0x3d,
  OUT, PALREG, 14, PALREG, 0x3e,
  OUT, PALREG, 15, PALREG, 0x3f,
  OUT, PALREG, 16, PALREG, 0x08,
  OUT, PALREG, 17, PALREG, 0x00,
  OUT, PALREG, 18, PALREG, 0x0f,
  OUT, PALREG, 19, PALREG, 0x00,

  /* Enable palette */
  OUT, PALREG, 0x20, 0, 0,

  /* End of table */
  DONE, 0, 0, 0, 0
};

#endif /* EGA_STANDARD */
