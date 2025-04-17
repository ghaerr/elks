/*
 * VGA Graphics for ELKS - C86, OWC and IA16 compilers
 *  Supports 16-color VGA and 256 color palette modes
 *
 * 1 Feb 2025 Greg Haerr C86 support
 * 13 Mar 2025 Added OWC ASM routines from Rafael Diniz
 * 16 Mar 2025 Added IA16
 */

#include <stdio.h>
#include <stdlib.h>
#include "graphics.h"
#include "vgalib.h"

int SCREENWIDTH;                /* initialized by graphics_open */
int SCREENHEIGHT;

int graphics_open(int mode)
{
    switch (mode) {
    case VGA_640x350x16:
        SCREENWIDTH = 640;
        SCREENHEIGHT = 350;
        set_bios_mode(mode);
        vga_init();
        break;
    case VGA_640x480x16:
        SCREENWIDTH = 640;
        SCREENHEIGHT = 480;
        set_bios_mode(mode);
        vga_init();
        break;
    case PAL_320x200x256:
        SCREENWIDTH = 320;
        SCREENHEIGHT = 200;
        set_bios_mode(mode);
        break;
    default:
        printf("Unsupported mode: %02x\n", mode);
        return -1;
    }
    return 0;
}

void graphics_close(void)
{
    set_bios_mode(TEXT_MODE);
}

#ifdef __WATCOMC__
static unsigned char mask[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

void vga_init(void)
{
    set_enable_sr(0xff);
    set_op(0);
    set_write_mode(0);
}

void drawpixel(int x, int y, int color)
{
    set_color(color);
    set_mask(mask[x & 7]);
    asm_orbyte((y << 6) + (y << 4) + (x >> 3)); /* y * 80 + x / 8 */
}

int readpixel(int x, int y)
{
    int c = 0;
    int offset = (y<<6) + (y<<4) + (x >> 3);    /* y * 80  + x / 8*/
    for (int plane=0; plane<4; plane++) {
        set_read_plane(plane);
        if (asm_getbyte(offset) & mask[x&7])
            c |= 1 << plane;
    }
    return c;
}

// Draw a horizontal line from x1,1 to x2,y including final point
void drawhline(int x1, int x2, int y, int c)
{
    set_color(c);
    unsigned int dst = (y<<6) + (y<<4) + (x1>>3);  /* y * 80 + x / 8 */
    if ((x1 >> 3) == (x2 >> 3)) {
        set_mask((0xff >> (x1 & 7)) & (0xff << (7 - (x2 & 7))));
        asm_orbyte(dst);
    } else {
        set_mask(0xff >> (x1 & 7));
        asm_orbyte(dst); dst++;
        set_mask(0xff);
        unsigned int last = (y<<6) + (y<<4) + (x2>>3);
        while (dst < last) {
            asm_orbyte(dst); dst++;
        }
        set_mask(0xff << (7 - (x2 & 7)));
        asm_orbyte(dst);
    }
}

void drawvline(int x, int y1, int y2, int c)
{
    while (y1 <= y2)
        drawpixel(x, y1++, c);
}
#endif /* __WATCOMC__ */

#if defined(__WATCOMC__) || defined(__ia16__)
/* draw 8 bits horizontally using XOR amd one or two memory writes */
static void drawbits(int x, int y, unsigned char bits)
{
    set_op(0x18);
    set_color(15);
    unsigned int dst = (y<<6) + (y<<4) + (x>>3);  /* y * 80 + x / 8 */
    if ((x & 7) == 0) {
        set_mask(bits);
        asm_orbyte(dst);
    } else {
        set_mask(bits >> ((x & 7)));
        asm_orbyte(dst); dst++;

        set_mask(bits << (8 - (x & 7)));
        asm_orbyte(dst);
    }
    set_op(0);
}

/* fast cursor draw for EGA/VGA hardware */
void vga_drawcursor(int x, int y, int height, unsigned short *mask)
{
    unsigned int bits;
    for (int i=0; i<height; i++) {
        bits = *mask++;
        if (bits >> 8)
            drawbits(x, y, bits >> 8);
        if (bits & 0xff)
            drawbits(x+8, y, bits);
        y++;
    }
}
#endif

void fillrect(int x1, int y1, int x2, int y2, int c)
{
    while(y1 <= y2)
        drawhline(x1, x2, y1++, c);
}

#ifdef __C86__

/* use BIOS to set video mode */
void set_bios_mode(int mode)
{
    asm(
        "push   si\n"
        "push   di\n"
        "push   ds\n"
        "push   es\n"
        "mov    ax,[bp+4]\n"    /* AH=0, AL=mode */
        "int    0x10\n"
        "pop    es\n"
        "pop    ds\n"
        "pop    di\n"
        "pop    si\n"
    );
}

int asm_getbyte(int offset)
{
    asm(
        "mov dx,ds\n"
        "mov bx,[bp+4]\n"
        "mov ax,#0xa000\n"
        "mov ds,ax\n"
        "mov al,[bx]\n"         /* offset */
        "xor ah,ah\n"
        "mov ds,dx\n"
    );
}

#if UNUSED
/* PAL write color byte at video offset */
void pal_writevid(unsigned int offset, int c)
{
    asm(
        "mov    dx,ds\n"
        "mov    ax,#0xA000\n"
        "mov    ds,ax\n"
        "mov    bx,[bp+4]\n"    /* offset */
        "mov    al,[bp+6]\n"    /* color */
        "mov    [bx],al\n"
        "mov    ds,dx\n"
    );
}

/* PAL draw a pixel at x, y with 8-bit color c */
void pal_drawpixel(int x,int y, int color)
{
    pal_writevid(y*SCREENWIDTH + x, color);
}

int pal_readpixel(int x, int y)
{
    return 0;       /* FIXME */
}

/* PAL draw horizontal line */
void pal_drawhline(int x1, int x2, int y, int c)
{
    int x, offset;

    offset = y*SCREENWIDTH + x1;
    while(x1++ <= x2)
        pal_writevid(offset++, c);
}

void pal_drawvline(int x, int y1, int y2, int c)
{
    while (y1 <= y2)
        pal_drawpixel(x, y1++, c);
}
#endif

#endif /* __C86__ */
