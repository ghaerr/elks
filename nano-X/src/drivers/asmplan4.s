; Copyright (c) 1999 Greg Haerr <greg@censoft.com>
; Copyright (c) 1991 David I. Bell
; Permission is granted to use, distribute, or modify this source,
; provided that this copyright notice remains intact.
;
; EGA/VGA Screen Driver 16 color 4 planes, higher speed MASM version
;
; Note: this file is a replacement for vgaplan4.c, when raw
; speed rather than portability is desired
;
; The algorithms for some of these routines are taken from the book:
; Programmer's Guide to PC and PS/2 Video Systems by Richard Wilton.
;
; Routines to draw pixels and lines for EGA/VGA resolutions.
; The drawing mode in the data/rotate register is not changed in this
; module, and must be changed as necessary by the callers.
;
PIXBYTES	= 80		; number of bytes in scan line


MSC = 1
;__MEDIUM__ = 1
	include asm.h
	.header

	.cextrn	gr_mode,word	; temp kluge devdraw.c graphics draw mode
	.dseg
mode_table:
	db	00h,18h,10h,08h	; vga draw modes

	.cseg

;
; int ega_init(PSD psd)
;
	.cproc	ega_init
	mov	ax,1		; success
	ret
	.cendp	ega_init

;
; Routine to draw a horizontal line.
; Called from C:
;	ega_drawhine(x1, x2, y, color);
;
;	works in the following EGA and VGA modes:
;	200 line 16 colors modes
;	350 line modes
;	640x480 16 color

; argument offsets from bp
x1	= arg1		; first X coordinate
x2	= arg1 + 2	; second X coordinate
y	= arg1 + 4	; second Y coordinate
color	= arg1 + 6	; pixel value

	.cproc	ega_drawhline
	push	bp		; setup stack frame and preserve registers
	mov	bp, sp
	push	si
	push	di
	push	es

	; configure the graphics controller

	mov	dx, 03ceh	; DX := Graphics Controller port address

	mov	al, #3		; set data rotate register
	lea	bx, mode_table
	add	bx, @gr_mode
	mov	ah, [bx]
	out	dx, ax

	mov 	ah, color[bp]	; pixel value
	xor	al, al		; Set/Reset register number (0)
	out	dx, ax

	mov	ax, 0f01h	; AH := bit plane mask for Enable Set/Reset
	out	dx, ax		; AL := Enable Set/Reset register number

	push	ds		; preserve DS

	mov	ax, y[bp]
	mov	bx, x1[bp]

	; compute pixel address
	mov	dx, offset PIXBYTES	; AX := [row * PIXBYTES]
	mul	dx
	mov 	cl, bl		; save low order column bits
	shr	bx, 1		; BX := [col / 8]
	shr	bx, 1
	shr	bx, 1
	add	bx, ax		; BX := [row * PIXBYTES] + [col / 8]
	and	cl, 07h		; CL := [col % 8]
	xor	cl, 07h		; CL := 7 - [col % 8]
	mov 	ah, 01h		; AH := 1 << [7 - [col % 8]]	[mask]
	mov	dx, 0a000h	; ES := EGA buffer segment address
	mov	es, dx
				; AH := bit mask
				; ES:BX -> video buffer
				; CL := number bits to shift left
	mov	di, bx		; ES:DI -> buffer
	mov 	dh, ah		; DH := unshifted bit mask for left byte

	not	dh
	shl	dh, cl		; DH := reverse bit mask for first byte
	not	dh		; DH := bit mask for first byte

	mov	cx, x2[bp]
	and	cl, 7
	xor	cl, 7		; CL := number of bits to shift left
	mov 	dl, 0ffh	; DL := unshifted bit mask for right byte
	shl	dl, cl		; DL := bit mask for last byte

	; determine byte offset of first and last pixel in the line

	mov	ax, x2[bp]	; AX := x2
	mov	bx, x1[bp]	; BX := x1

	mov 	cl, 3		; bits to convert pixels to bytes

	shr	ax, cl		; AX := byte offset of X2
	shr	bx, cl		; BX := byte offset of X1
	mov	cx, ax
	sub	cx, bx		; CX := [number of bytes in line] - 1

	; get Graphics Controller port address into DX

	mov	bx, dx		; BH := bit mask for first byte
				; BL := bit mask for last byte
	mov	dx, 03ceh	; DX := Graphics Controller port
	mov 	al, 8		; AL := Bit mask Register number

	; make video buffer addressable through DS:SI

	push	es
	pop	ds
	mov	si, di		; DS:SI -> video buffer

	; set pixels in leftmost byte of the line

	or	bh, bh
	js	L43		; jump if byte-aligned [x1 is leftmost]

	or	cx, cx
	jnz	L42		; jump if more than one byte in the line

	and	bl, bh		; BL := bit mask for the line
	jmp short L44

L42:	mov 	ah, bh		; AH := bit mask for first byte
	out	dx, ax		; update graphics controller

	movsb			; update bit planes
	dec	cx

	; use a fast 8086 machine instruction to draw the remainder of the line

L43:	mov 	ah, 0ffh	; AH := bit mask
	out	dx, ax		; update Bit Mask register
	rep movsb		; update all pixels in the line

	; set pixels in the rightmost byte of the line

L44:	mov 	ah, bl		; AH := bit mask for last byte
	out	dx, ax		; update Graphics Controller
	movsb			; update bit planes

	pop	ds		; restore ds


	; restore default Graphics Controller state and return to caller
	;;xor	ax, ax		; AH := 0, AL := 0
	;;out	dx, ax		; restore Set/Reset register
	;;inc	ax		; AH := 0, AL := 1
	;;out	dx, ax		; restore Enable Set/Reset register
	;;mov	ax, 0ff08h	; AH := 0xff, AL := 0
	;;out	dx, ax		; restore Bit Mask register

	pop	es
	pop	di
	pop	si
	pop	bp
	ret
	.cendp	ega_drawhline


;
; Routine to draw a vertical line.
; Called from C:
;	ega_drawvline(x, y1, y2, color);
;
;	works in the following EGA and VGA modes:
;	200 line 16 colors modes
;	350 line modes
;	640x480 16 color

; argument offsets from bp
x	= arg1		; first X coordinate
y1	= arg1 + 2	; first Y coordinate
y2	= arg1 + 4	; second Y coordinate
color	= arg1 + 6	; pixel value

	.cproc	ega_drawvline
	push	bp		; setup stack frame and preserve registers
	mov	bp, sp
	push	ds

	; configure the graphics controller

	mov	dx, 03ceh	; DX := Graphics Controller port address

	mov	al, #3		; set data rotate register
	lea	bx, mode_table
	add	bx, @gr_mode
	mov	ah, [bx]
	out	dx, ax

	mov 	ah, color[bp]	; pixel value
	xor	al, al		; Set/Reset register number (0)
	out	dx, ax

	mov	ax, 0f01h	; AH := bit plane mask for Enable Set/Reset
	out	dx, ax		; AL := Enable Set/Reset register number

	; prepare to draw vertical line

	mov	ax, y1[bp]	; AX := y1
	mov	cx, y2[bp]	; BX := y2
	;;mov	cx, bx
	sub	cx, ax		; CX := dy
	;;jge	L311		; jump if dy >= 0
	;;neg	cx		; force dy >= 0
	;;mov	ax, bx		; AX := y2

L311:	inc	cx		; CX := number of pixels to draw
	mov	bx, x[bp]	; BX := x
	push	cx		; save register

	; compute pixel address
	push	dx
	mov	dx, offset PIXBYTES	; AX := [row * PIXBYTES]
	mul	dx
	mov 	cl, bl		; save low order column bits
	shr	bx, 1		; BX := [col / 8]
	shr	bx, 1
	shr	bx, 1
	add	bx, ax		; BX := [row * PIXBYTES] + [col / 8]
	and	cl, 07h		; CL := [col % 8]
	xor	cl, 07h		; CL := 7 - [col % 8]
	mov 	ah, 01h		; AH := 1 << [7 - [col % 8]]	[mask]
	mov	dx, 0a000h	; DS := EGA buffer segment address
	mov	ds, dx
	pop	dx
				; AH := bit mask
				; DS:BX -> video buffer
				; CL := number bits to shift left

	; set up Graphics controller

	shl	ah, cl		; AH := bit mask in proper position
	mov 	al, 08h		; AL := Bit Mask register number
	out	dx, ax

	pop	cx		; restore register

	; draw the line

	mov	dx, offset PIXBYTES	; increment for video buffer
L1111:	or	[bx], al	; set pixel
	add	bx, dx		; increment to next line
	loop	L1111

	; restore default Graphics Controller state and return to caller
	;;xor	ax, ax		; AH := 0, AL := 0
	;;out	dx, ax		; restore Set/Reset register
	;;inc	ax		; AH := 0, AL := 1
	;;out	dx, ax		; restore Enable Set/Reset register
	;;mov	ax, 0ff08h	; AH := 0xff, AL := 0
	;;out	dx, ax		; restore Bit Mask register

	pop	ds
	pop	bp
	ret
	.cendp	ega_drawvline


;
; Routine to set an individual pixel value.
; Called from C like:
;	ega_drawpixel(x, y, color);
;

; argument offsets from bp
x	= arg1		; X coordinate
y	= arg1+2	; Y coordinate
color	= arg1+4	; pixel value

	.cproc	ega_drawpixel
	push	bp
	mov	bp, sp

	mov	dx, 03ceh	; graphics controller port address
	mov	al, #3		; set data rotate register
	lea	bx, mode_table
	add	bx, @gr_mode
	mov	ah, [bx]
	out	dx, ax

	mov	cx, x[bp]		; ECX := x
	mov	ax, y[bp]		; EAX := y

	mov	dx, offset PIXBYTES	; AX := [y * PIXBYTES]
	mul	dx

	mov	bx, cx		; BX := [x / 8]
	shr	bx, 1
	shr	bx, 1
	shr	bx, 1

	add	bx, ax		; BX := [y * PIXBYTES] + [x / 8]

	and	cl, 07h		; CL := [x % 8]
	xor	cl, 07h		; CL := 7 - [x % 8]
	mov 	ch, 01h		; CH := 1 << [7 - [x % 8]]	[mask]
	shl	ch, cl

	mov	dx, 03ceh	; graphics controller port address

	;;required for old code
	;;mov	ax, 0205h	; select write mode 2
	;;out	dx, ax		; [load value 2 into mode register 5]

	; new code
	xor	ax,ax		; set color register 0
	mov	ah,[bp+8]	; color pixel value
	out	dx,ax

	; original code
	mov 	al, 08h		; set the bit mask register
	mov 	ah, ch		; [load bit mask into register 8]
	out	dx, ax

	push	ds
	mov	ax, 0a000h	; DS := EGA buffer segment address
	mov	ds, ax

	; new code
	or	[bx],al		; quick rmw to set pixel

	;;the following fails under ELKS without cli/sti
	;;using ES works though.  Code changed to use single
	;;rmw above rather than write mode 2, but the
	;;reason for this failure is still unknown...
	;;cli
	;;mov 	al, [bx]	; dummy read to latch bit planes
	;;mov	al, color[bp]	; pixel value
	;;mov 	[bx], al	; write pixel back to bit planes
	;;sti

	pop	ds		; restore registers and return

	mov	ax, 0005h	; restore default write mode 0
	out	dx, ax		; [load value 0 into mode register 5]
	;;mov	ax, 0ff08h	; restore default bit mask
	;;out	dx, ax		; [load value ff into register 8]

	pop	bp
	ret
	.cendp	ega_drawpixel

;
; Routine to read the value of an individual pixel.
; Called from C like:
; 	color = ega_readpixel(x, y);
;

; argument offsets from bp
x	= arg1			; X coordinate
y	= arg1+2		; Y coordinate

	.cproc	ega_readpixel
	push	bp
	mov	bp, sp
	push	si
	push	ds

	mov	ax, y[bp]	; EAX := y
	mov	bx, x[bp]	; EBX := x
	mov	dx, offset PIXBYTES	; AX := [y * PIXBYTES]
	mul	dx

	mov 	cl, bl		; save low order column bits
	shr	bx, 1		; BX := [x / 8]
	shr	bx, 1
	shr	bx, 1

	add	bx, ax		; BX := [y * PIXBYTES] + [x / 8]

	and	cl, 07h		; CL := [x % 8]
	xor 	cl, 07h		; CL := 7 - [x % 8]

	mov	dx, 0a000h	; DS := EGA buffer segment address
	mov	ds, dx

	mov 	ch, 01h		; CH := 1 << [7 - [col % 8]]  [mask]
	shl	ch, cl		; CH := bit mask in proper position

	mov	si, bx		; DS:SI -> region buffer byte
	xor	bl, bl		; BL is used to accumulate the pixel value

	mov	dx, 03ceh	; DX := Graphics Controller port
	mov	ax, 0304h	; AH := initial bit plane number
				; AL := Read Map Select register number

L112:	out	dx, ax		; select bit plane
	mov 	bh, [si]	; BH := byte from current bit plane
	and	bh, ch		; mask one bit
	neg	bh		; bit 7 of BH := 1 if masked bit = 1
				; bit 7 of BH := 0 if masked bit = 0
	rol	bx, 1		; bit 0 of BL := next bit from pixel value
	dec	ah		; AH := next bit plane number
	jge	L112

	xor	ax, ax		; AL := pixel value
	mov 	al, bl

	pop	ds
	pop	si
	pop	bp	
	ret
	.cendp	ega_readpixel

	.cend
	end
