/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * ELKS utility routines for Micro-Windows drivers
 */
#include "../device.h"
#include "vgaplan4.h"

/*
 * Return the byte at far address
 */
unsigned char
GETBYTE_FP(FARADDR addr)
{
#asm
	push	bp
	mov	bp,sp
	push	ds

	mov	bx,[bp+4]	! bx = lo addr
	mov	ax,[bp+6]	! ds = hi addr
	mov	ds,ax
	mov	al,[bx]		! get byte at ds:bx
	xor	ah,ah

	pop	ds
	pop	bp
#endasm
}

/*
 * Put the byte at far address
 */
void
PUTBYTE_FP(FARADDR addr,unsigned char val)
{
#asm
	push	bp
	mov	bp,sp
	push	ds

	mov	bx,[bp+4]	! bx = lo addr
	mov	ax,[bp+6]	! ds = hi addr
	mov	ds,ax
	mov	al,[bp+8]	! al = val
	mov	[bx],al		! put byte at ds:bx

	pop	ds
	pop	bp
#endasm
}

/*
 * Read-modify-write the byte at far address
 */
void
RMW_FP(FARADDR addr)
{
#asm
	push	bp
	mov	bp,sp
	push	ds

	mov	bx,[bp+4]	! bx = lo addr
	mov	ax,[bp+6]	! ds = hi addr
	mov	ds,ax
	or	[bx],al		! rmw byte at ds:bx, al value doesn't matter

	pop	ds
	pop	bp
#endasm
}

/*
 * Or the byte at far address
 */
void
ORBYTE_FP(FARADDR addr,unsigned char val)
{
#asm
	push	bp
	mov	bp,sp
	push	ds

	mov	bx,[bp+4]	! bx = lo addr
	mov	ax,[bp+6]	! ds = hi addr
	mov	ds,ax
	mov	al,[bp+8]	! al = val
	or	[bx],al		! or byte at ds:bx

	pop	ds
	pop	bp
#endasm
}

/*
 * And the byte at far address
 */
void
ANDBYTE_FP(FARADDR addr,unsigned char val)
{
#asm
	push	bp
	mov	bp,sp
	push	ds

	mov	bx,[bp+4]	! bx = lo addr
	mov	ax,[bp+6]	! ds = hi addr
	mov	ds,ax
	mov	al,[bp+8]	! al = val
	and	[bx],al		! and byte at ds:bx

	pop	ds
	pop	bp
#endasm
}

/*
 * Input byte from i/o port
 */
int
inportb(int port)
{
#asm
	push	bp
	mov	bp,sp

	mov	dx,[bp+4]	! dx = port
	in	al,dx		! input byte
	xor	ah,ah

	pop	bp
#endasm
}

/*
 * Output byte to i/o port
 */
void
outportb(int port,unsigned char data)
{
#asm
	push	bp
	mov	bp,sp

	mov	dx,[bp+4]	! dx = port
	mov	al,[bp+6]	! al = data
	out	dx,al

	pop	bp
#endasm
}

/*
 * Output word i/o port
 */
void
outport(int port,int data)
{
#asm
	push	bp
	mov	bp,sp

	mov	dx,[bp+4]	! dx = port
	mov	ax,[bp+6]	! ax = data
	out	dx,ax

	pop	bp
#endasm
}

/*
 * es:bp = int10(int ax,int bx)
 *  Call video bios using interrupt 10h
 */
FARADDR
int10(int ax,int bx)
{
#asm
	push	bp
	mov	bp,sp
	push	es
	push	ds
	push	si
	push	di

	mov	ax,[bp+4]	! get first arg
	mov	bx,[bp+6]	! get second arg
	int	$10
	mov	dx,es		! return es:bp
	mov	ax,bp

	pop	di
	pop	si
	pop	ds
	pop	es
	pop	bp
#endasm
}

/* poll the keyboard*/
int
kbpoll(void)
{
#asm
	mov	ah,1			! read, no remove
	int	$16
	jz	nordy			! no chars ready
	mov	ax,1			! chars ready
	ret
nordy:	xor	ax,ax			! no chars ready
#endasm
}

/* wait and read a kbd char when ready*/
int
kbread(void)
{
#asm
	xor	ah,ah			! read and remove
	int	$16			! return ax
#endasm
}

/* return kbd shift status*/
int
kbflags(void)
{
#asm
	mov	ah,2			! get shift status
	int	$16
	mov	ah,0			! low bits only for now...
#endasm
}
