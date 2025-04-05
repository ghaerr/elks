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

int SCREENWIDTH;                /* initialized by graphics_open */
int SCREENHEIGHT;

#ifdef __WATCOMC__
static unsigned char mask[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

void set_mode(int mode);
#pragma aux set_mode parm [ax] =                \
    "int 10h",                                  \
    modify [ ax ];

void vga_init(void);
#pragma aux vga_init =                          \
    "mov dx,0x03ce",                            \
    "mov ax,0xff01",                            \
    "out dx,ax",                                \
    "mov ax,0x0003",                            \
    "out dx,ax",                                \
    "mov ax,0x0005",                            \
    "out dx,ax",                                \
    modify [ ax dx ];

void set_mask(unsigned int mask);
#pragma aux set_mask parm [ax] =                \
    "mov dx,0x03ce",                            \
    "mov ah,al",                                \
    "mov al,8",                                 \
    "out dx,ax",                                \
    modify [ ax dx ];

void set_color(unsigned int color);
#pragma aux set_color parm [ax] =               \
    "mov dx,0x03ce",                            \
    "mov ah,al",                                \
    "xor al,al",                                \
    "out dx,ax",                                \
    modify [ ax dx ];

void set_read_plane(unsigned int plane);
#pragma aux set_read_plane parm [ax] =          \
    "mov dx,0x03ce",                            \
    "mov ah,al",                                \
    "mov al,4",                                 \
    "out dx,ax",                                \
    modify [ ax dx ];

void asm_drawpixel(unsigned int offset);
#pragma aux asm_drawpixel parm [ax] =           \
    "mov cx,ds",                                \
    "mov bx,ax",                                \
    "mov ax,0xa000",                            \
    "mov ds,ax",                                \
    "or [bx],al",                               \
    "mov ds,cx",                                \
    modify [ ax bx cx dx ];

int get_byte(unsigned int offset);
#pragma aux get_byte parm [ax] =                \
    "mov cx,ds",                                \
    "mov bx,ax",                                \
    "mov ax,0xa000",                            \
    "mov ds,ax",                                \
    "mov al,[bx]",                              \
    "xor ah,ah",                                \
    "mov ds,cx",                                \
    modify [ ax bx cx dx ];
#endif

#ifdef __ia16__
#define set_mode(mode)                          \
    asm volatile (                              \
        "int $0x10"                             \
        : /* no output */                       \
        : "a" ((unsigned short)(mode))          \
    )

#define vga_init_unused()                       \
    asm volatile (                              \
        "mov $0x03ce,%%dx\n"                    \
        "mov $0xff01,%%ax\n"                    \
        "out %%ax,%%dx\n"                       \
        "mov $0x0003,%%ax\n"                    \
        "out %%ax,%%dx\n"                       \
        "mov $0x0005,%%ax\n"                    \
        "out %%ax,%%dx\n"                       \
        : /* no output */                       \
        : /* no input */                        \
        : "a", "d"                              \
        )

#define set_mask(mask)                          \
    asm volatile (                              \
        "mov $0x03ce,%%dx\n"                    \
        "mov %%al,%%ah\n"                       \
        "mov $8,%%al\n"                         \
        "out %%ax,%%dx\n"                       \
        : /* no output */                       \
        : "a" ((unsigned short)(mask))          \
        : "d"                                   \
        )

#define set_color(color)                        \
    asm volatile (                              \
        "mov $0x03ce,%%dx\n"                    \
        "mov %%al,%%ah\n"                       \
        "xor %%al,%%al\n"                       \
        "out %%ax,%%dx\n"                       \
        : /* no output */                       \
        : "a" ((unsigned short)(color))         \
        : "d"                                   \
        )

#define set_read_plane(plane)                   \
    asm volatile (                              \
        "mov $0x03ce,%%dx\n"                    \
        "mov %%al,%%ah\n"                       \
        "mov $4,%%al\n"                         \
        "out %%ax,%%dx\n"                       \
        : /* no output */                       \
        : "a" ((unsigned short)(plane))         \
        : "d"                                   \
        )

#define asm_drawpixel(offset)                   \
    asm volatile (                              \
        "mov %%ds,%%cx\n"                       \
        "mov $0xa000,%%ax\n"                    \
        "mov %%ax,%%ds\n"                       \
        "or %%al,(%%bx)\n"                      \
        "mov %%cx,%%ds\n"                       \
        : /* no output */                       \
        : "b" ((unsigned short)(offset))        \
        : "a", "c", "d"                         \
        )

#define get_byte(offset)                        \
    __extension__ ({                            \
    unsigned short _v;                          \
    asm volatile (                              \
        "mov %%ds,%%cx\n"                       \
        "mov $0xa000,%%ax\n"                    \
        "mov %%ax,%%ds\n"                       \
        "mov (%%bx),%%al\n"                     \
        "xor %%ah,%%ah\n"                       \
        "mov %%cx,%%ds\n"                       \
        : "=a" (_v)                             \
        : "b" ((unsigned short)(offset))        \
        : "c", "d"                              \
        );                                      \
    _v; })
#endif

#ifdef __C86__
/* use BIOS to set video mode */
static void set_mode(int mode)
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

/* PAL write color byte at video offset */
static void pal_writevid(unsigned int offset, int c)
{
    asm(
        "push   ds\n"
        "push   bx\n"
        "mov    ax,#0xA000\n"
        "mov    ds,ax\n"
        "mov    bx,[bp+4]\n"    /* offset */
        "mov    al,[bp+6]\n"    /* color */
        "mov    [bx],al\n"
        "pop    bx\n"
        "pop    ds\n"
    );
}
#endif

int graphics_open(int mode)
{
    switch (mode) {
    case VGA_640x350x16:
        SCREENWIDTH = 640;
        SCREENHEIGHT = 350;
        set_mode(mode);
        vga_init();
        break;
    case VGA_640x480x16:
        SCREENWIDTH = 640;
        SCREENHEIGHT = 480;
        set_mode(mode);
        vga_init();
        break;
    case PAL_320x200x256:
        SCREENWIDTH = 320;
        SCREENHEIGHT = 200;
        set_mode(mode);
        break;
    default:
        printf("Unsupported mode: %02x\n", mode);
        return -1;
    }
    return 0;
}

void graphics_close(void)
{
    set_mode(TEXT_MODE);
}

#ifdef __WATCOMC__
void drawpixel(int x, int y, int color)
{
    set_color(color);
    set_mask(mask[x & 7]);
    asm_drawpixel((y<<6) + (y<<4) + (x >> 3));  /* =y*80 FIXME changes with resolution */
}

int readpixel(int x, int y)
{
    int c = 0;
    for (int plane=0; plane<4; plane++) {
        set_read_plane(plane);
        if (get_byte((y<<6) + (y<<4) + (x >> 3)) & mask[x&7])   /* = y * 80 */
            c |= 1 << plane;
    }
    return c;
}

#define EGA_BASE ((char __far *)0xA0000000L)

// Draw a horizontal line from x1,1 to x2,y including final point
void drawhline(int x1, int x2, int y, int c)
{
    set_color(c);
    char __far *dst = EGA_BASE + (x1>>3) + (y<<6) + (y<<4); /* y * 80 */
    if ((x1 >> 3) == (x2 >> 3)) {
        set_mask((0xff >> (x1 & 7)) & (0xff << (7 - (x2 & 7))));
        *dst |= 1;
    } else {
        set_mask(0xff >> (x1 & 7));
        *dst++ |= 1;
        set_mask(0xff);
        char __far *last = EGA_BASE + (x2>>3) + (y<<6) + (y<<4);
        while (dst < last)
            *dst++ |= 1;
        set_mask(0xff << (7 - (x2 & 7)));
        *dst |= 1;
    }
}

void drawvline(int x, int y1, int y2, int c)
{
    while (y1 <= y2)
        drawpixel(x, y1++, c);
}
#endif /* __WATCOMC__ */

void fillrect(int x1, int y1, int x2, int y2, int c)
{
    while(y1 <= y2)
        drawhline(x1, x2, y1++, c);
}

#ifdef __C86__
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
