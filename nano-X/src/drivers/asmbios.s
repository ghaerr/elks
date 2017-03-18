; Copyright (c) 1999 Greg Haerr <greg@censoft.com>
;
; int10 bios video function access
; int16 bios keyboard access
;
; assembly language routines for MSDOS Microsoft C v5.10
;
; assemble with masm -Mx -D__MEDIUM__
;


MSC = 1
;__MEDIUM__ = 1
	include asm.h
	.header

	.cseg

;
; es:bp = int10(int ax,int bx)
;	Call video bios using interrupt 10h
;
	.cproc	int10
	push	bp
	mov	bp,sp
	push	es
	push	ds
	push	si
	push	di

	mov	ax,arg1[bp]	; get first arg
	mov	bx,arg1+2[bp]	; get second arg
	int	10h
	mov	dx,es		; return es:bp
	mov	ax,bp

	pop	di
	pop	si
	pop	ds
	pop	es
	pop	bp
	ret
	.cendp	int10

;
; int kbpoll(void) - poll keyboard for char ready
;
	.cproc	kbpoll
	mov	ah,01h			; read, no remove
	int	16h
	jz	$9			; no chars ready
	mov	ax,1			; chars ready
	ret
$9:	xor	ax,ax			; no chars ready
	ret
	.cendp	kbpoll
;
; int kbread(void) - wait and read a kbd char when ready
;
	.cproc	kbread
	mov	ah,00h			; read and remove
	int	16h			; return ax
	ret
	.cendp	kbread
;
; int kbflags(void) - return kbd shift status
;
	.cproc	kbflags
	mov	ah,02h			; get shift status
	int	16h
	mov	ah,0			; low bits only for now...
	ret
	.cendp	kbflags

	.cend
	end
