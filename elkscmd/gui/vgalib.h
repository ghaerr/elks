#ifndef VGALIB_H
#define VGALIB_H

/* vgalib.h */

/*
 * VGA hardware manipulation from C
 *
 * A set of macros for accessing VGA registers for ia-elf-gcc,
 * OpenWatcom and C86 compilers.
 *
 * Functions/Macros:
 *  set_color(color)            // 03ce REG 0 Set/Reset Register (color for write mode 0)
 *  set_enable_sr(flag)         // 03ce REG 1 Enable Set/Reset Register (forces REG 0)
 *  set_color_compare(color)    // 03ce REG 2 Color Compare Register
 *  set_op(op)                  // 03ce REG 3 Data Rotate Register (nop, xor)
 *  set_read_plane(plane)       // 03ce REG 4 Read Map Select Register
 *  set_write_mode(mode)        // 03ce REG 5 Graphics Mode Register (write mode 0)
 *  set_color_dont_care(color)  // 03ce REG 7 Color Don't Care Register
 *  set_mask(mask)              // 03ce REG 8 Bit Mask Register
 *  set_write_planes(mask)      // 03c4 REG 2 Memory Plane Write Enable Register
 *
 *  void set_bios_mode(mode)    // set BIOS graphics/text mode
 *  void asm_orbyte(int offset) // OR byte at A000:offset
 *  unsigned short  asm_getbyte(int offset)// read byte at A000:offset
 *
 * Some defaults:
 *  set_color(0)                // REG 0
 *  set_enable_sr(0xff)         // REG 1
 *  set_op(0)                   // REG 3
 *  set_read_plane(0)           // REG 4
 *  set_write_mode(0)           // REG 5
 *  set_mask(0xff)              // REG 8
 *  set_write_planes(0x0f)      // REG 2
 *
 * 6 Apr 2025 Greg Haerr
 */

#define EGA_BASE 0xA000         /* segment address of EGA/VGA video memory */
#define MK_FP(seg,off)          \
        ((void __far *)((((unsigned long)(seg)) << 16) | ((unsigned int)(off))))

#ifdef __ia16__

#define set_color(color)                        \
    asm volatile (                              \
        "out %%ax,%%dx\n"                       \
        : /* no output */                       \
        : "a" ((unsigned short)(((color)<<8)|0))\
        , "d" (0x03ce)                          \
        )

#define set_enable_sr(flag)                     \
    asm volatile (                              \
        "out %%ax,%%dx\n"                       \
        : /* no output */                       \
        : "a" ((unsigned short)(((flag)<<8)|1)) \
        , "d" (0x03ce)                          \
        )

#define set_color_compare(color)                \
    asm volatile (                              \
        "out %%ax,%%dx\n"                       \
        : /* no output */                       \
        : "a" ((unsigned short)(((color)<<8)|2))\
        , "d" (0x03ce)                          \
        )

#define set_op(op)                              \
    asm volatile (                              \
        "out %%ax,%%dx\n"                       \
        : /* no output */                       \
        : "a" ((unsigned short)(((op)<<8)|3))   \
        , "d" (0x03ce)                          \
        )

#define set_read_plane(plane)                   \
    asm volatile (                              \
        "out %%ax,%%dx\n"                       \
        : /* no output */                       \
        : "a" ((unsigned short)(((plane)<<8)|4))\
        , "d" (0x03ce)                          \
        )

#define set_write_mode(mode)                    \
    asm volatile (                              \
        "out %%ax,%%dx\n"                       \
        : /* no output */                       \
        : "a" ((unsigned short)(((mode)<<8)|5)) \
        , "d" (0x03ce)                          \
        )

#define set_mask(mask)                          \
    asm volatile (                              \
        "out %%ax,%%dx\n"                       \
        : /* no output */                       \
        : "a" ((unsigned short)(((mask)<<8)|8)) \
        , "d" (0x03ce)                          \
        )
#define set_color_dont_care(color)              \
    asm volatile (                              \
        "out %%ax,%%dx\n"                       \
        : /* no output */                       \
        : "a" ((unsigned short)(((color)<<8)|7))\
        , "d" (0x03ce)                          \
        )

#define set_write_planes(mask)                  \
    asm volatile (                              \
        "out %%ax,%%dx\n"                       \
        : /* no output */                       \
        : "a" ((unsigned short)(((mask)<<8)|2)) \
        , "d" (0x03c4)                          \
        )

#define set_bios_mode(mode)                     \
    asm volatile (                              \
        "int $0x10"                             \
        : /* no output */                       \
        : "a" ((unsigned short)(mode))          \
    )

#define asm_orbyte(offset)                      \
    asm volatile (                              \
        "mov %%ds,%%dx\n"                       \
        "mov %%ax,%%ds\n"                       \
        "or %%al,(%%bx)\n"                      \
        "mov %%dx,%%ds\n"                       \
        : /* no output */                       \
        : "a" (0xa000)                          \
        , "b" ((unsigned short)(offset))        \
        : "d","dl","dh"                         \
        )

#define asm_getbyte(offset)                     \
    __extension__ ({                            \
    unsigned short _v;                          \
    asm volatile (                              \
        "mov %%ds,%%dx\n"                       \
        "mov %%ax,%%ds\n"                       \
        "mov (%%bx),%%al\n"                     \
        "xor %%ah,%%ah\n"                       \
        "mov %%dx,%%ds\n"                       \
        : "=a" (_v)                             \
        : "a" (0xa000)                          \
        , "b" ((unsigned short)(offset))        \
        : "d","dl","dh"                         \
        );                                      \
    _v; })

#endif /* __ia16__ */

#ifdef __WATCOMC__

void set_color(unsigned int color);
#pragma aux set_color parm [ax] =               \
    "mov dx,0x03ce",                            \
    "mov ah,al",                                \
    "xor al,al",                                \
    "out dx,ax",                                \
    modify [ ax dx ];

void set_enable_sr(unsigned int flag);
#pragma aux set_enable_sr parm [ax] =           \
    "mov dx,0x03ce",                            \
    "mov ah,al",                                \
    "mov al,1",                                 \
    "out dx,ax",                                \
    modify [ ax dx ];

void set_color_compare(unsigned int color);
#pragma aux set_color_compare parm [ax] =       \
    "mov dx,0x03ce",                            \
    "mov ah,al",                                \
    "mov al,2",                                 \
    "out dx,ax",                                \
    modify [ ax dx ];

void set_op(unsigned int op);
#pragma aux set_op parm [ax] =                  \
    "mov dx,0x03ce",                            \
    "mov ah,al",                                \
    "mov al,3",                                 \
    "out dx,ax",                                \
    modify [ ax dx ];

void set_read_plane(unsigned int plane);
#pragma aux set_read_plane parm [ax] =          \
    "mov dx,0x03ce",                            \
    "mov ah,al",                                \
    "mov al,4",                                 \
    "out dx,ax",                                \
    modify [ ax dx ];

void set_write_mode(unsigned int mode);
#pragma aux set_write_mode parm [ax] =          \
    "mov dx,0x03ce",                            \
    "mov ah,al",                                \
    "mov al,5",                                 \
    "out dx,ax",                                \
    modify [ ax dx ];

void set_color_dont_care(unsigned int color);
#pragma aux set_color_dont_care parm [ax] =     \
    "mov dx,0x03ce",                            \
    "mov ah,al",                                \
    "mov al,7",                                 \
    "out dx,ax",                                \
    modify [ ax dx ];

void set_mask(unsigned int mask);
#pragma aux set_mask parm [ax] =                \
    "mov dx,0x03ce",                            \
    "mov ah,al",                                \
    "mov al,8",                                 \
    "out dx,ax",                                \
    modify [ ax dx ];

void set_write_planes(unsigned int mask);
#pragma aux set_write_planes parm [ax] =        \
    "mov dx,0x03c4",                            \
    "mov ah,al",                                \
    "mov al,2",                                 \
    "out dx,ax",                                \
    modify [ ax dx ];

void asm_orbyte(unsigned int offset);
#pragma aux asm_orbyte parm [ax] =              \
    "mov dx,ds",                                \
    "mov bx,ax",                                \
    "mov ax,0xa000",                            \
    "mov ds,ax",                                \
    "or [bx],al",                               \
    "mov ds,dx",                                \
    modify [ ax bx dx ];

int asm_getbyte(unsigned int offset);
#pragma aux asm_getbyte parm [ax] =             \
    "mov dx,ds",                                \
    "mov bx,ax",                                \
    "mov ax,0xa000",                            \
    "mov ds,ax",                                \
    "mov al,[bx]",                              \
    "xor ah,ah",                                \
    "mov ds,dx",                                \
    modify [ ax bx dx ];

void set_bios_mode(int mode);
#pragma aux set_bios_mode parm [ax] =           \
    "int 10h",                                  \
    modify [ ax ];

#endif /* __WATCOMC__ */

#ifdef __C86__
/*
 * NOTE: the following C86 macros only work with constant arguments!
 */

#define set_color(color)                        \
    asm("mov dx,*0x03ce\n"                      \
        "xor al,al\n"                           \
        "mov ah,*" #color "\n"                  \
        "out dx,ax\n"                           \
    )

#define set_enable_sr(flag)                     \
    asm("mov dx,*0x03ce\n"                      \
        "mov al,*1\n"                           \
        "mov ah,*" #flag "\n"                   \
        "out dx,ax\n"                           \
    )

#define set_color_compare(color)                \
    asm("mov dx,*0x03ce\n"                      \
        "mov al,*2\n"                           \
        "mov ah,*" #color "\n"                  \
        "out dx,ax\n"                           \
    )

#define set_op(op)                              \
    asm("mov dx,*0x03ce\n"                      \
        "mov al,*3\n"                           \
        "mov ah,*" #op "\n"                     \
        "out dx,ax\n"                           \
    )

#define set_read_plane(plane)                   \
    asm("mov dx,*0x03ce\n"                      \
        "mov al,*4\n"                           \
        "mov ah,*" #plane "\n"                  \
        "out dx,ax\n"                           \
    )

#define set_write_mode(mode)                    \
    asm("mov dx,*0x03ce\n"                      \
        "mov al,*5\n"                           \
        "mov ah,*" #mode "\n"                   \
        "out dx,ax\n"                           \
    )

#define set_color_dont_care(color)              \
    asm("mov dx,*0x03ce\n"                      \
        "mov al,*7\n"                           \
        "mov ah,*" #color "\n"                  \
        "out dx,ax\n"                           \
    )

#define set_mask(mask)                          \
    asm("mov dx,*0x03ce\n"                      \
        "mov al,*8\n"                           \
        "mov ah,*" #mask "\n"                   \
        "out dx,ax\n"                           \
    )

#define set_write_planes(mask)                  \
    asm("mov dx,*0x03c4\n"                      \
        "mov al,*2\n"                           \
        "mov ah,*" #mask "\n"                   \
        "out dx,ax\n"                           \
    )

int asm_getbyte(int offset);
void set_bios_mode(int mode);

#endif /* __C86__ */

#endif
