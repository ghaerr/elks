/*******************************************************************
 * memmove function for far data. The number of bytes transferred  *
 * must be <= 32767 ( ie. fit in an signed int ). That's because   *
 * of ( you guessed it... ) the braindead segmented architecture   *
 * Saku Airila 1996                                                *
 *******************************************************************/

#include <linuxmt/mm.h>

static void blt_back(unsigned,unsigned,unsigned,unsigned,unsigned);
static void blt_forth(unsigned,unsigned,unsigned,unsigned,unsigned);

void far_memmove(unsigned sseg, unsigned soff, unsigned dseg, unsigned doff,
		 unsigned bytes)
{
    if (bytes) {
	sseg += (soff >> 4);
	soff &= 0xf;
	dseg += (doff >> 4);
	doff &= 0xf;

	if ((sseg < dseg) || ((sseg == dseg) && (soff < doff))) {
	    --bytes;
	    blt_forth(sseg, soff+bytes, dseg, doff+bytes, bytes+1);
	} else {
	    blt_back(sseg, soff, dseg, doff, bytes );
	}
    }
}

#ifndef S_SPLINT_S
#asm
				! blt_back( sseg, soff, dseg, doff, bytes )
				! for to < from
	.text
	.even

_blt_back:
	push	bp
	mov	bp, sp
	push	ax
	push	es
	push	ds
	push	cx	
	push	si
	push	di
	pushf
	mov	ax, [bp+4]
	mov	ds, ax
	mov	si, [bp+6]
	mov	ax, [bp+8]
	mov	es, ax
	mov	di, [bp+10]
	mov	cx, [bp+12]
	cld
	rep
	movsb
	popf
	pop	di
	pop	si
	pop	cx
	pop	ds
	pop	es
	pop	ax
	pop	bp
	ret
				! blt_forth( sseg, soff, dseg, doff, bytes )
				! for to > from 
	.even

_blt_forth:
	push	bp
	mov	bp, sp
	push	ax
	push	es
	push	ds
	push	cx	
	push	si
	push	di
	pushf
	mov	ax, [bp+4]
	mov	ds, ax
	mov	si, [bp+6]
	mov	ax, [bp+8]
	mov	es, ax
	mov	di, [bp+10]
	mov	cx, [bp+12]
	std
	rep
	movsb
	popf
	pop	di
	pop	si
	pop	cx
	pop	ds
	pop	es
	pop	ax
	pop	bp
	ret

#endasm
#endif
