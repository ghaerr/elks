// ELKS utility routines for Micro-Windows drivers
// Copyright (c) 1999, 2019 Greg Haerr <greg@censoft.com>

	.code16
	.text

//
// Return the byte at far address
//
//unsigned char GETBYTE_FP(FARADDR addr)
//
	.global	GETBYTE_FP
GETBYTE_FP:
	push	%bp
	mov		%sp,%bp
	push	%ds

	mov		4(%bp),%bx	// bx = lo addr
	mov		6(%bp),%ax	// ds = hi addr
	mov		%ax,%ds
	mov		(%bx),%al	// get byte at ds:bx
	xor		%ah,%ah

	pop		%ds
	pop		%bp
	ret

//
// Put the byte at far address
//
//void PUTBYTE_FP(FARADDR addr,unsigned char val)
//
	.global	PUTBYTE_FP
PUTBYTE_FP:
	push	%bp
	mov		%sp,%bp
	push	%ds

	mov		4(%bp),%bx	// bx = lo addr
	mov		6(%bp),%ax	// ds = hi addr
	mov		%ax,%ds
	mov		8(%bp),%al	// al = val
	mov		%al,(%bx)	// put type at ds:bx

	pop		%ds
	pop		%bp
	ret

//
// Read-modify-write the byte at far address
//
//void RMW_FP(FARADDR addr)
	.global	RMW_FP
RMW_FP:
	push	%bp
	mov		%sp,%bp
	push	%ds

	mov		4(%bp),%bx	// bx = lo addr
	mov		6(%bp),%ax	// ds = hi addr
	mov		%ax,%ds
	or		%al,(%bx)	// rmw byte at ds:bx, al value doesn't matter

	pop		%ds
	pop		%bp
	ret

//
// Or the byte at far address
//
//void ORBYTE_FP(FARADDR addr,unsigned char val)
	.global	ORBYTE_FP
ORBYTE_FP:
	push	%bp
	mov		%sp,%bp
	push	%ds

	mov		4(%bp),%bx	// bx = lo addr
	mov		6(%bp),%ax	// ds = hi addr
	mov		%ax,%ds
	mov		8(%bp),%al	// al = val
	or		%al,(%bx)	// or byte at ds:bx

	pop		%ds
	pop		%bp
	ret

//
// And the byte at far address
//
//void ANDBYTE_FP(FARADDR addr,unsigned char val)
	.global	ANDBYTE_FP
ANDBYTE_FP:
	push	%bp
	mov		%sp,%bp
	push	%ds

	mov		4(%bp),%bx	// bx = lo addr
	mov		6(%bp),%ax	// ds = hi addr
	mov		%ax,%ds
	mov		8(%bp),%al	// al = val
	and		%al,(%bx)	// and byte at ds:bx

	pop		%ds
	pop		%bp
	ret

//
// Input byte from i/o port
//
//int inportb(int port)
	.global	inportb
inportb:
	push	%bp
	mov		%sp,%bp

	mov		4(%bp),%dx	// dx = port
	in		%dx,%al		// input byte
	xor		%ah,%ah

	pop		%bp
	ret

//
// Output byte to i/o port
//
//void outportb(int port,unsigned char data)
	.global	outportb
outportb:
	push	%bp
	mov		%sp,%bp

	mov		4(%bp),%dx	// dx = port
	mov		6(%bp),%al	// al = data
	out		%al,%dx

	pop		%bp
	ret

//
// Output word i/o port
//
//void outport(int port,int data)
	.global	outport
outport:
	push	%bp
	mov		%sp,%bp

	mov		4(%bp),%dx	// dx = port
	mov		6(%bp),%ax	// ax = data
	out		%ax,%dx

	pop		%bp
	ret

//
// es:bp = int10(int ax,int bx)
//  Call video bios using interrupt 10h
//
//FARADDR int10(int ax,int bx)
	.global	int10
int10:
	push	%bp
	mov		%sp,%bp
	push	%es
	push	%ds
	push	%si
	push	%di

	mov		4(%bp),%ax	// get first arg
	mov		6(%bp),%bx	// get second arg
	int		$10
	mov		%es,%dx		// return es:bp
	mov		%bp,%ax

	pop		%di
	pop		%si
	pop		%ds
	pop		%es
	pop		%bp
	ret

// poll the keyboard*/
//int kbpoll(void)
	.global	kbpoll
kbpoll:
	mov		$1,%ah		// read, no remove
	int		$16
	jz		nordy		// no chars ready
	mov		$1,%ax		// chars ready
	ret
nordy:
	xor		%ax,%ax		// no chars ready
	ret

// wait and read a kbd char when ready*/
//int kbread(void)
	.global	kbread
kbread:
	xor		%ah,%ah		// read and remove
	int		$16			// return ax
	ret

// return kbd shift status*/
//int kbflags(void)
	.global	kbflags
kbflags:
	mov		$2,%ah		// get shift status
	int		$16
	mov		$0,%ah		// low bits only for now...
	ret
