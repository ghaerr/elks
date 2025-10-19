/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Header file for EGA/VGA 16 color 4 planes screen driver
 * Added functions for Hercules access
 */
#define HAVEBLIT	1	/* =0 to exclude blitting in vgaplan4 drivers*/

#if LINUX
#include <sys/io.h>		/* for outb def's, requires -O */
#define HAVEFARPTR	1
#define FAR
#endif

#if MSDOS
#define HAVEFARPTR	1	/* compiler has _far extension*/
#define FAR		_far
#endif


/* make far ptr*/
#define MK_FP(seg,ofs)	((FARADDR) \
			   (((unsigned long)(seg) << 16L) | (unsigned)(ofs)))

#if HAVEFARPTR
/* far ptr access to screen*/
typedef volatile unsigned char FAR * FARADDR;

/* get byte at address*/
#define GETBYTE_FP(addr)	(*(FARADDR)(addr))

/* put byte at address*/
#define PUTBYTE_FP(addr,val)	((*(FARADDR)(addr)) = (val))

/* read-modify-write at address*/
#define RMW_FP(addr)		((*(FARADDR)(addr)) |= 1)

/* or byte at address*/
#define ORBYTE_FP(addr,val)	((*(FARADDR)(addr)) |= (val))

/* and byte at address*/
#define ANDBYTE_FP(addr,val)	((*(FARADDR)(addr)) &= (val))
#else

/* for bcc with no _far extension*/
typedef unsigned long	FARADDR;

/* get byte at address*/
extern unsigned char GETBYTE_FP(FARADDR);

/* put byte at address*/
extern void PUTBYTE_FP(FARADDR,unsigned char);

/* read-modify-write at address*/
extern void RMW_FP(FARADDR);

/* or byte at address*/
extern void ORBYTE_FP(FARADDR,unsigned char);

/* and byte at address*/
extern void ANDBYTE_FP(FARADDR,unsigned char);
#endif


#if MSDOS
#define outb(val,port)	outp(port,val)
#endif

#if ELKS
#include <arch/io.h>
#elif 0
#define outb(val,port)	outportb(port,val)
#define outw(val,port)	outport(port,val)

extern int  inportb(int port);
extern void outportb(int port,unsigned char data);
extern void outport(int port,int data);
#endif


/* external routines*/
FARADDR		int10(int ax,int bx);

/* external routines implementing planar ega/vga access*/

/* vgaplan4.c portable C, asmplan4.s asm, or ELKS asm elkplan4.c driver*/
int		ega_init(PSD psd);
void 		ega_drawpixel(PSD psd,unsigned int x,unsigned int y,PIXELVAL c);
PIXELVAL 	ega_readpixel(PSD psd,unsigned int x,unsigned int y);
void		ega_drawhorzline(PSD psd,unsigned int x1,unsigned int x2,
			unsigned int y,PIXELVAL c);
void		ega_drawvertline(PSD psd,unsigned int x,unsigned int y1,
			unsigned int y2, PIXELVAL c);
void	 	ega_blit(PSD dstpsd, COORD dstx, COORD dsty, COORD w, COORD h,
			PSD srcpsd, COORD srcx, COORD srcy, int op);

/* vgainit.c - direct hw init*/
void		ega_hwinit(void);
void		ega_hwterm(void);

#if HAVEBLIT
/* mempl4.c*/
void 	 mempl4_drawpixel(PSD psd, COORD x, COORD y, PIXELVAL c);
PIXELVAL mempl4_readpixel(PSD psd, COORD x, COORD y);
void 	 mempl4_drawhorzline(PSD psd, COORD x1, COORD x2, COORD y, PIXELVAL c);
void 	 mempl4_drawvertline(PSD psd, COORD x, COORD y1, COORD y2, PIXELVAL c);
void 	 mempl4_to_mempl4_blit(PSD dstpsd,COORD dstx,COORD dsty,COORD w,COORD h,
		PSD srcpsd,COORD srcx,COORD srcy,int op);
void 	 vga_to_mempl4_blit(PSD dstpsd,COORD dstx,COORD dsty,COORD w,COORD h,
		PSD srcpsd, COORD srcx, COORD srcy, int op);
void 	 mempl4_to_vga_blit(PSD dstpsd,COORD dstx,COORD dsty,COORD w,COORD h,
		PSD srcpsd,COORD srcx,COORD srcy,int op);
void 	 vga_to_vga_blit(PSD dstpsd,COORD dstx,COORD dsty,COORD w,COORD h,
		PSD srcpsd,COORD srcx,COORD srcy,int op);
#endif

/* Program the Set/Reset Register for drawing in color COLOR for write mode 0. */
#define set_color(c)            { outw (0|((c)<<8), 0x3ce); }

/* Set the Enable Set/Reset Register. */
#define set_enable_sr(mask) 	{ outw (1|((mask)<<8), 0x3ce); }

/* Select the Bit Mask Register on the Graphics Controller. */
#define select_mask()           { outb (8, 0x3ce); }

/* Program the Bit Mask Register to affect only the pixels selected in MASK.
   The Bit Mask Register must already have been selected with select_mask (). */
#define set_mask(mask)          { outb (mask, 0x3cf); }

/* Set the Data Rotate Register.  Bits 0-2 are rotate count, bits 3-4
   are logical operation (0=NOP, 1=AND, 2=OR, 3=XOR). */
#define set_op(op)              { outw (3|((op)<<8), 0x3ce); }

/* Set the Memory Plane Write Enable register. */
#define set_write_planes(mask)	{ outw (2|((mask)<<8), 0x3c4); }

/* Set the Read Map Select register. */
#define set_read_plane(plane)	{ outw (4|((plane)<<8), 0x3ce); }

/* Set the Graphics Mode Register.  The write mode is in bits 0-1,
   the read mode is in bit 3. */
#define set_mode(mode)          { outw (5|((mode)<<8), 0x3ce); }
