/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Copyright (c) 1991 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Alternate EGA/VGA Screen Driver Init, direct hw programming
 */
#include "../device.h"
#include "vgaplan4.h"


/* Define one and only one of the following to be nonzero*/
#define	VGA_ET4000	0	/* TSENG LABS ET4000 chip 800x600*/
#define	VGA_STANDARD	1	/* standard VGA 640x480*/
#define	EGA_STANDARD	0	/* standard EGA 640x350*/

#define	DONE	0
#define	IN	1
#define	OUT	2

#define	RAM_SCAN_LINES	32	/* number of scan lines in fonts in RAM */
#define	FONT_CHARS	256	/* number of characters in font tables */
#define	CHAR_WIDTH	8	/* number of pixels for character width */
#define	EGA_BASE	0xa0000	/* location of enhanced display memory */


#define	PALREG	0x3c0
#define	SEQREG	0x3c4
#define	SEQVAL	0x3c5
#define	GRREG	0x3ce
#define	GRVAL	0x3cf
#define	ATTRREG	0x3da
#define	CRTCREG	0x3d4
#define	CRTCVAL	0x3d5

#define	GENREG1	0x3c2
#define	GENREG2	0x3cc
#define	GENREG3	0x3ca

#define	DATA_ROTATE	3	/* register number for data rotate */

typedef struct {
  int action;
  int port1;
  int data1;
  int port2;
  int data2;
} REGIO;

/* extern data*/
extern FARADDR 		rom_char_addr;		/* address of ROM font*/
extern int		ROM_CHAR_HEIGHT;	/* ROM character height*/

/* local data*/
extern REGIO 		graphics_on[];
extern REGIO 		graph_off[];

/* entry points*/
void		ega_hwinit(void);
void		ega_hwterm(void);

/* local routines*/
static void writeregs(REGIO *rp);
static void out_word(unsigned int p,unsigned int d);
static void setmode(MODE mode);

void
ega_hwinit(void)
{
	writeregs(graphics_on);
}

void
ega_hwterm(void)
{
#if 1
  /* bios 80x25 text mode*/
  int10(3, 0);
#else
  FARADDR	srcoffset;
  FARADDR	destoffset;
  int 		data;
  int 		ch;
  int 		row;

  setmode(MODE_SET);

  /* Copy character table from ROM back into bit plane 2 before turning
   * off graphics.
   */
  out_word(SEQREG, 0x0100);	/* syn reset */
  out_word(SEQREG, 0x0402);	/* cpu writes only to map 2 */
  out_word(SEQREG, 0x0704);	/* sequential addressing */
  out_word(SEQREG, 0x0300);	/* clear synchronous reset */

  out_word(GRREG, 0x0204);	/* select map 2 for CPU reads */
  out_word(GRREG, 0x0005);	/* disable odd-even addressing */

  srcoffset = rom_char_addr;
  destoffset = MK_FP(EGA_BASE, 0);
  for (ch = 0; ch < FONT_CHARS; ch++) {
	for(row = 0; row < ROM_CHAR_HEIGHT; row++) {
		data = GETBYTE_FP(srcoffset++);
		PUTBYTE_FP(destoffset++, data);
	}
	destoffset += (RAM_SCAN_LINES - ROM_CHAR_HEIGHT);
  }

  /* Finally set the registers back for text mode. */
  writeregs(graph_off);
#endif
}

/* Set the graphics registers as indicated by the given table */
static void
writeregs(REGIO *rp)
{
  for (; rp->action != DONE; rp++) {
	switch (rp->action) {
	    case IN:
			inp(rp->port1);
			break;
	    case OUT:
			outp(rp->port1, rp->data1);
			if (rp->port2)
				outp(rp->port2, rp->data2);
			break;
	}
  }
}

/* Output a word to an I/O port. */
static void
out_word(unsigned int p,unsigned int d)
{
  outp(p, d & 0xff);
  outp(p + 1, (d >> 8) & 0xff);
}


/* Values for the data rotate register to implement drawing modes. */
static unsigned char mode_table[MODE_MAX + 1] = {
  0x00, 0x18, 0x10, 0x08
};

/* Set the drawing mode.
 * This is either SET, OR, AND, or XOR.
 */
static void
setmode(MODE mode)
{
  if (mode > MODE_MAX)
  	return;
  outp(GRREG, DATA_ROTATE);
  outp(GRVAL, mode_table[mode]);
}


#if VGA_ET4000

/* VGA 800x600 16-color graphics (BIOS mode 0x29).
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
  OUT, CRTCREG, 0, CRTCVAL, 0x7a,
  OUT, CRTCREG, 1, CRTCVAL, 0x63,
  OUT, CRTCREG, 2, CRTCVAL, 0x64,
  OUT, CRTCREG, 3, CRTCVAL, 0x1d,
  OUT, CRTCREG, 4, CRTCVAL, 0x68,
  OUT, CRTCREG, 5, CRTCVAL, 0x9a,
  OUT, CRTCREG, 6, CRTCVAL, 0x78,
  OUT, CRTCREG, 7, CRTCVAL, 0xf0,
  OUT, CRTCREG, 8, CRTCVAL, 0x00,
  OUT, CRTCREG, 9, CRTCVAL, 0x60,
  OUT, CRTCREG, 10, CRTCVAL, 0x00,
  OUT, CRTCREG, 11, CRTCVAL, 0x00,
  OUT, CRTCREG, 12, CRTCVAL, 0x00,
  OUT, CRTCREG, 13, CRTCVAL, 0x00,
  OUT, CRTCREG, 14, CRTCVAL, 0x00,
  OUT, CRTCREG, 15, CRTCVAL, 0x00,
  OUT, CRTCREG, 16, CRTCVAL, 0x5c,
  OUT, CRTCREG, 17, CRTCVAL, 0x8e,
  OUT, CRTCREG, 18, CRTCVAL, 0x57,
  OUT, CRTCREG, 19, CRTCVAL, 0x32,
  OUT, CRTCREG, 20, CRTCVAL, 0x00,
  OUT, CRTCREG, 21, CRTCVAL, 0x5b,
  OUT, CRTCREG, 22, CRTCVAL, 0x75,
  OUT, CRTCREG, 23, CRTCVAL, 0xc3,
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
  OUT, CRTCREG, 0, CRTCVAL, 0x5f,	/* horiz total */
  OUT, CRTCREG, 1, CRTCVAL, 0x4f,	/* horiz end */
  OUT, CRTCREG, 2, CRTCVAL, 0x50,	/* horiz blank */
  OUT, CRTCREG, 3, CRTCVAL, 0x82,	/* end blank */
  OUT, CRTCREG, 4, CRTCVAL, 0x55,	/* horiz retrace */
  OUT, CRTCREG, 5, CRTCVAL, 0x81,	/* end retrace */
  OUT, CRTCREG, 6, CRTCVAL, 0xbf,	/* vert total */
  OUT, CRTCREG, 7, CRTCVAL, 0x1f,	/* overflows */
  OUT, CRTCREG, 8, CRTCVAL, 0x00,	/* row scan */
  OUT, CRTCREG, 9, CRTCVAL, 0x4f,	/* max scan line */
  OUT, CRTCREG, 10, CRTCVAL, 0x00,	/* cursor start */
  OUT, CRTCREG, 11, CRTCVAL, 0x0f,	/* cursor end */
  OUT, CRTCREG, 12, CRTCVAL, 0x0e,	/* start high addr */
  OUT, CRTCREG, 13, CRTCVAL, 0xb0,	/* low addr */
  OUT, CRTCREG, 14, CRTCVAL, 0x16,	/* cursor high */
  OUT, CRTCREG, 15, CRTCVAL, 0x30,	/* cursor low */
  OUT, CRTCREG, 16, CRTCVAL, 0x9c,	/* vert retrace */
  OUT, CRTCREG, 17, CRTCVAL, 0x8e,	/* retrace end */
  OUT, CRTCREG, 18, CRTCVAL, 0x8f,	/* vert end */
  OUT, CRTCREG, 19, CRTCVAL, 0x28,	/* offset */
  OUT, CRTCREG, 20, CRTCVAL, 0x1f,	/* underline */
  OUT, CRTCREG, 21, CRTCVAL, 0x96,	/* vert blank */
  OUT, CRTCREG, 22, CRTCVAL, 0xb9,	/* end blank */
  OUT, CRTCREG, 23, CRTCVAL, 0xa3,	/* crt mode */
  OUT, CRTCREG, 24, CRTCVAL, 0xff,	/* line compare */

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

#endif


#if VGA_STANDARD

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
  OUT, CRTCREG, 0, CRTCVAL, 0x5f,	/* horiz total */
  OUT, CRTCREG, 1, CRTCVAL, 0x4f,	/* horiz end */
  OUT, CRTCREG, 2, CRTCVAL, 0x50,	/* horiz blank */
  OUT, CRTCREG, 3, CRTCVAL, 0x82,	/* end blank */
  OUT, CRTCREG, 4, CRTCVAL, 0x55,	/* horiz retrace */
  OUT, CRTCREG, 5, CRTCVAL, 0x81,	/* end retrace */
  OUT, CRTCREG, 6, CRTCVAL, 0xbf,	/* vert total */
  OUT, CRTCREG, 7, CRTCVAL, 0x1f,	/* overflows */
  OUT, CRTCREG, 8, CRTCVAL, 0x00,	/* row scan */
  OUT, CRTCREG, 9, CRTCVAL, 0x4f,	/* max scan line */
  OUT, CRTCREG, 10, CRTCVAL, 0x00,	/* cursor start */
  OUT, CRTCREG, 11, CRTCVAL, 0x0f,	/* cursor end */
  OUT, CRTCREG, 12, CRTCVAL, 0x0e,	/* start high addr */
  OUT, CRTCREG, 13, CRTCVAL, 0xb0,	/* low addr */
  OUT, CRTCREG, 14, CRTCVAL, 0x16,	/* cursor high */
  OUT, CRTCREG, 15, CRTCVAL, 0x30,	/* cursor low */
  OUT, CRTCREG, 16, CRTCVAL, 0x9c,	/* vert retrace */
  OUT, CRTCREG, 17, CRTCVAL, 0x8e,	/* retrace end */
  OUT, CRTCREG, 18, CRTCVAL, 0x8f,	/* vert end */
  OUT, CRTCREG, 19, CRTCVAL, 0x28,	/* offset */
  OUT, CRTCREG, 20, CRTCVAL, 0x1f,	/* underline */
  OUT, CRTCREG, 21, CRTCVAL, 0x96,	/* vert blank */
  OUT, CRTCREG, 22, CRTCVAL, 0xb9,	/* end blank */
  OUT, CRTCREG, 23, CRTCVAL, 0xa3,	/* crt mode */
  OUT, CRTCREG, 24, CRTCVAL, 0xff,	/* line compare */

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

#endif


#if EGA_STANDARD

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
  OUT, CRTCREG, 0, CRTCVAL, 0x5b,	/* horiz total */
  OUT, CRTCREG, 1, CRTCVAL, 0x4f,	/* horiz end */
  OUT, CRTCREG, 2, CRTCVAL, 0x53,	/* horiz blank */
  OUT, CRTCREG, 3, CRTCVAL, 0x37,	/* end blank */
  OUT, CRTCREG, 4, CRTCVAL, 0x51,	/* horiz retrace */
  OUT, CRTCREG, 5, CRTCVAL, 0x5b,	/* end retrace */
  OUT, CRTCREG, 6, CRTCVAL, 0x6c,	/* vert total */
  OUT, CRTCREG, 7, CRTCVAL, 0x1f,	/* overflows */
  OUT, CRTCREG, 8, CRTCVAL, 0x00,	/* row scan */
  OUT, CRTCREG, 9, CRTCVAL, 0x0d,	/* max scan line */
  OUT, CRTCREG, 10, CRTCVAL, 0x00,	/* cursor start */
  OUT, CRTCREG, 11, CRTCVAL, 0x0f,	/* cursor end */
  OUT, CRTCREG, 12, CRTCVAL, 0x00,	/* start high addr */
  OUT, CRTCREG, 13, CRTCVAL, 0x00,	/* low addr */
  OUT, CRTCREG, 14, CRTCVAL, 0x00,	/* cursor high */
  OUT, CRTCREG, 15, CRTCVAL, 0x00,	/* cursor low */
  OUT, CRTCREG, 16, CRTCVAL, 0x5e,	/* vert retrace */
  OUT, CRTCREG, 17, CRTCVAL, 0x2b,	/* retrace end */
  OUT, CRTCREG, 18, CRTCVAL, 0x5d,	/* vert end */
  OUT, CRTCREG, 19, CRTCVAL, 0x28,	/* offset */
  OUT, CRTCREG, 20, CRTCVAL, 0x0f,	/* underline */
  OUT, CRTCREG, 21, CRTCVAL, 0x5e,	/* vert blank */
  OUT, CRTCREG, 22, CRTCVAL, 0x0a,	/* end blank */
  OUT, CRTCREG, 23, CRTCVAL, 0xa3,	/* crt mode */
  OUT, CRTCREG, 24, CRTCVAL, 0xff,	/* line compare */

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

#endif
