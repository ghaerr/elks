/*******************************************************************
 * memmove function for far data. The number of bytes transferred  *
 * must be <= 32767 ( ie. fit in an signed int ). That's because   *
 * of ( you guessed it... ) the braindead segmented architecture   *
 * Saku Airila 1996                                                *
 *******************************************************************/

#include <linuxmt/types.h>

static void blt_back(__u16,__u16,__u16,__u16,__u16);
static void blt_forth(__u16,__u16,__u16,__u16,__u16);

void far_memmove(__u16 sseg,__u16 soff,__u16 dseg,__u16 doff,__u16 bytes)
{
    __u32 slin, dlin;

    slin = ((__u32) sseg) << 4 + ((__u32) soff);
    dlin = ((__u32) dseg) << 4 + ((__u32) doff);

    if( slin < dlin ) {

	/* The idea is to get most of the ptr info in the offset part. The
	 * number of bytes to transfer must be added to the pointers because
	 * here we are copying in reverse order starting from the tail.
	 */

	slin += bytes;
	dlin += bytes;

	soff = (__u16) (slin & 0xFFFF);
	doff = (__u16) (dlin & 0xFFFF);

	sseg = (__u16) ((slin >> 4) & 0xF000);
	dseg = (__u16) ((dlin >> 4) & 0xF000);

	while( soff < bytes + 1 ) {
	    soff += 0x1000;
	    sseg -= 0x0100;
	}

	while( doff < bytes + 1 ) {
	    doff += 0x1000;
	    dseg -= 0x0100;
	}

	blt_forth( sseg, soff, dseg, doff, ++bytes );

    } else {

	/* And here, we want most of it in the segment part */

	soff = (__u16) (slin & 0x0F);
	doff = (__u16) (dlin & 0x0F);

	sseg = (__u16) ((slin >> 4) & 0xFFFF);
	dseg = (__u16) ((dlin >> 4) & 0xFFFF);

	blt_back( sseg, soff, dseg, doff, bytes );
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
	.text

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
