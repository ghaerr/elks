
!
! SYS_SIZE is the number of clicks (16 bytes) to be loaded.
! 0x7F00 is 0x7F000 bytes = 508kB, more than enough for current
! versions of linux which compress the kernel
!
#include <linuxmt/config.h>
SYSSIZE = DEF_SYSSIZE
!
!	bootsect.s		Copyright (C) 1991, 1992 Linus Torvalds
!	modified by Drew Eckhardt
!	modified by Bruce Evans (bde)
! 	modified by Alan Cox for Linux/ELKS and 8088 compatiblity
!	modified for Linux/16 by Chad Page 
!
! bootsect.s is loaded at 0x7c00 by the bios-startup routines, and moves
! itself out of the way to address 0x90000, and jumps there.
!
! bde - should not jump blindly, there may be systems with only 512K low
! memory.  Use int 0x12 to get the top of memory, etc.
!
! It then loads 'setup' directly after itself (0x90200), and the system
! at 0x10000, using BIOS interrupts. 
!
! NOTE! currently system is at most (8*65536-4096) bytes long. This should 
! be no problem, even in the future. I want to keep it simple. This 508 kB
! kernel size should be enough, especially as this doesn't contain the
! buffer cache as in minix (and especially now that the kernel is 
! compressed :-)
!
! The loader has been made as simple as possible, and continuous
! read errors will result in a unbreakable loop. Reboot by hand. It
! loads pretty fast by getting whole tracks at a time whenever possible.

.text

SETUPSECS = 4				! nr of setup-sectors
BOOTSEG   = 0x07C0			! original address of boot-sector
INITSEG   = DEF_INITSEG			! we move boot here - out of the way
SETUPSEG  = DEF_SETUPSEG
SYSSEG    = DEF_SYSSEG			! system loaded at 0x10000 (65536).
SYSSEGB   = DEF_SYSSEG + 2

! ROOT_DEV & SWAP_DEV are now written by "build".
ROOT_DEV = 0
SWAP_DEV = 0
#ifndef SVGA_MODE
#define SVGA_MODE ASK_VGA
#endif
#ifndef RAMDISK
#define RAMDISK 0
#endif 
#ifndef CONFIG_ROOT_RDONLY
#define CONFIG_ROOT_RDONLY 0
#endif

! ld86 requires an entry symbol. This may as well be the usual one.
.globl	_main
_main:
#if 0 /* hook for debugger, harmless unless BIOS is fussy (old HP) */
	int	3
#endif
	mov	ax,#BOOTSEG
	mov	ds,ax
	mov	ax,#INITSEG
	mov	es,ax
	mov	cx,#256
	xor	si,si
	xor	di,di
	cld
	rep
	movsw
	jmpi	go,INITSEG

! ax and es already contain INITSEG

go:	mov	di,#0x4000-12	
! 0x4000 is arbitrary value >= length of
! bootsect + length of setup + room for stack
! 12 is disk parm size

! bde - changed 0xff00 to 0x4000 to use debugger at 0x6400 up (bde).  We
! wouldn't have to worry about this if we checked the top of memory.  Also
! my BIOS can be configured to put the wini drive tables in high memory
! instead of in the vector table.  The old stack might have clobbered the
! drive table.

	mov	ds,ax
	mov	ss,ax		! put stack at INITSEG:0x4000-12.
	mov	sp,di
/*
 *	Many BIOS's default disk parameter tables will not 
 *	recognize multi-sector reads beyond the maximum sector number
 *	specified in the default diskette parameter tables - this may
 *	mean 7 sectors in some cases.
 *
 *	Since single sector reads are slow and out of the question,
 *	we must take care of this by creating new parameter tables
 *	(for the first disk) in RAM.  We will set the maximum sector
 *	count to 36 - the most we will encounter on an ED 2.88.  
 *
 *	High doesn't hurt.  Low does.
 *
 *	Segments are as follows: ds=es=ss=cs - INITSEG,
 */

	mov	bx,#0x78		
! 0:bx is parameter table address
	push	ds
	push	es
	xor 	ax,ax
	mov	es,ax			! 
	seg es
	lds	si,[bx]
	pop	es

! ds:si is source

	mov	cl,#6			
! copy 12 bytes
	cld
	push	di

	rep
	movsw

	pop	di
	pop	ds
					! what kind of instruction is this?
	movb	4[di],*36		! patch sector count

	push	ds
!	xor	ax,ax			ax still 0
	mov	ds,ax
	mov	[bx],di
	mov	2[bx],es
	pop	ds

! load the setup-sectors directly after the bootblock.
! Note that 'es' is already set up.
! Also cx is 0 from rep movsw above.

load_setup:
	xor	ah,ah			! reset FDC 
	xor	dl,dl
	int 	0x13	

	xor	dx, dx			! drive 0, head 0
	mov	cx,#0x0002		! sector 2, track 0
	mov	bx,#0x0200		! address = 512, in INITSEG
	mov	ah,#0x02		! service 2, nr of sectors
	mov	al,setup_sects		! (assume all on head 0, track 0)
	int	0x13			! read it
	jnc	ok_load_setup		! ok - continue

	push	ax			! dump error code 
	call	print_nl
	mov	bp, sp
	call	print_hex
	pop	ax	
	
	jmp	load_setup

ok_load_setup:

! Get disk drive parameters, specifically nr of sectors/track

#if 0

! bde - the Phoenix BIOS manual says function 0x08 only works for fixed
! disks.  It doesn't work for one of my BIOS's (1987 Award).  It was
! fatal not to check the error code.

	xor	dl,dl
	mov	ah,#0x08		! AH=8 is get drive parameters
	int	0x13
	xor	ch,ch
#else

! It seems that there is no BIOS call to get the number of sectors.  Guess
! 36 sectors if sector 36 can be read, 18 sectors if sector 18 can be read,
! 15 if sector 15 can be read.  Otherwise guess 9.

	mov	si,#disksizes		! table of sizes to try

probe_loop:
	lodsb
	cbw				! extend to word
	mov	sectors, ax
	cmp	si,#disksizes_end
	jae	got_sectors		! if all else fails, try 9
	xchg	ax, cx			! cx = track and sector
	xor	dx, dx			! drive 0, head 0
	xor	bl, bl
	mov	bh,setup_sects
	inc	bh
	shl	bh,#1			! address after setup (es = cs)
	mov	ax,#0x0201		! service 2, 1 sector
	int	0x13
	jc	probe_loop		! try next value

#endif

got_sectors:

! Restore es

!	mov	ax,#INITSEG
!	mov	es,ax

! Print some inane message

!	mov	ah,#0x03		! read cursor pos
!	xor	bh,bh
!	int	0x10			
!	
!	mov	cx,#6
!	mov	bl,#0x0007		! page 0, attribute 7 (normal)
!	mov	bp,#msg1
!	mov	ax,#0x1301		! write string, move cursor
!	int	0x10

	mov	cx,#msg1end-msg1
	mov 	si,#msg1
nxt_chr:
	lodsb
	call print_chr
	loop	nxt_chr


! ok, we've written the message, now
! we want to load the system (at 0x10000)

	mov	ax,#SYSSEG
	mov	es,ax		! segment of 0x010000
	call	read_it
	call	kill_motor
	mov	al,#':
	call 	print_chr
	call	print_nl

! After that we check which root-device to use. If the device is
! defined (!= 0), nothing is done and the given device is used.
! Otherwise, one of /dev/fd0H2880 (2,32) or /dev/PS0 (2,28) or /dev/at0 (2,8),
! depending on the number of sectors we pretend to know we have.
! This is not valid under ELKS - ajr 13th Oct 97

!	seg cs
!	mov	ax,root_dev
!	or	ax,ax
!	jne	root_defined
!	seg cs
!	mov	bx,sectors
!	mov	ax,#0x0208		! /dev/ps0 - 1.2Mb
!	cmp	bx,#15
!	je	root_defined
!	mov	al,#0x1c		! /dev/PS0 - 1.44Mb
!	cmp	bx,#18
!	je	root_defined
!	mov	al,#0x20		! /dev/fd0H2880 - 2.88Mb
!	cmp	bx,#36
!	je	root_defined
!	mov	al,#0			! /dev/fd0 - autodetect
!root_defined:
!	seg cs
!	mov	root_dev,ax

! after that (everything loaded), we jump to
! the setup-routine loaded directly after
! the bootblock:
	jmpi	0,SETUPSEG

! This routine loads the system at address 0x10000, making sure
! no 64kB boundaries are crossed. We try to load it as fast as
! possible, loading whole tracks whenever we can.
!
! in:	es - starting address segment (normally 0x1000)
!
head:	.word 0			! current head
track:	.word 0			! current track

!dx=sectors read of current track
!bx=offset in memory of block
!si=sectors read to block
!al=number to read
!
read_it:
	mov	dx,setup_sects
	inc	dx
	mov 	ax,es
	test 	ax,#0x0fff
die:	jne 	die			! es must be at 64kB boundary
	xor 	si,si		! zero sectors read
rp_read:
	mov ax,es
	sub ax,#SYSSEG		
	cmp ax,syssize		! have we loaded all yet?
	jbe ok1_read
	ret
ok1_read:
	mov ax,sectors		! nr of sectors/track

	sub ax,dx		! remaining of track

	mov cx,ax		! cx= remaining

	add cx,si		! boundary check
	cmp cl,#128
	jbe ok2_read		! 
				! to much-> fill block
	mov ax,#128		! ax=0
	sub ax,si		! so much may be read

	
ok2_read:
	mov bx,si
	mov cx,#9
	shl bx,cl
	call read_track		! do it
	mov cx,ax		! cl=read blocks

	add ax,dx		! ax=new sectors

	cmp ax,sectors		! track done?
	jne ok3_read
	mov ax,#1		! yes
	sub ax,head		
	jne ok4_read		! next head
	inc track		! next track
ok4_read:
	mov head,ax
	xor ax,ax
ok3_read:
	mov dx,ax

	add si,cx
	cmp si,#128		
	jne rp_read

	mov ax,es
	add ah,#0x10
	mov es,ax

	xor si,si
	jmp rp_read


read_track:
	push 	dx
	push	ax
	push 	bx	
	
	mov	al, #'. 	! loading... message 2e = .
	call	print_chr

	pop	bx
	pop	ax
	push 	ax
	push	bx

	mov	cx,dx
	mov	dx,track
	inc	cx
	mov	ch,dl
	mov	dx,head
	mov	dh,dl
	and	dx,#0x0100
	mov	ah,#2
	
	push	dx				! save for error dump
	push	cx
	push	bx
	push	ax

	int	0x13			
!ah=02h al=nr sectors to read
!ch=cylinder
!cl=sector
!dh=head
!dl=drive
!es:bx=buffer

	jc	bad_rt
	add	sp, #8
	pop	bx
	pop	ax
	pop	dx
	ret

bad_rt: push	ax				! save error code
!	push sectors
	call	print_all			! ah = error, al = read
	
	
	xor ah,ah
	xor dl,dl
	int 0x13
	

	add	sp, #10
		
	pop	bx
	pop	ax
	pop	dx
	jmp read_track
/*
 *	print_all is for debugging purposes.  
 *	It will print out all of the registers.  The assumption is that this is
 *	called from a routine, with a stack frame like
 *	dx 
 *	cx
 *	bx
 *	ax
 *	error
 *	ret <- sp
 *
*/
 
print_all:
	mov	cx, #5		! error code + 4 registers
	mov	bp, sp	

no_reg:
	push	cx		! save count left
	add	bp, #2		! next register
	call	print_hex	! print it
	pop	cx
	loop	print_loop
	ret

print_loop:
	call	print_nl	! nl for readability

	
	mov	ax, #0xe05 + 'A - 1
	sub	al, cl
	int	0x10

	mov	al, #'X
	int	0x10

	mov	al, #':
	int	0x10
	jmp 	no_reg

print_nl:
	mov	al, #0x0d	! CR
	call	print_chr
	mov	al, #0xa	! LF
!	int 	0x10
	call	print_chr
	ret

/*
 *	print_hex is for debugging purposes, and prints the word
 *	pointed to by ss:bp in hexadecimal.
*/

print_hex:
!	mov	bl,#7
	mov	cx, #4		! 4 hex digits
	mov	dx, [bp]	! load word into dx
print_digit:
	push	cx
	mov 	cl,#4
	rol	dx, cl		! rotate so that lowest 4 bits are used
	pop	cx
!	mov	ax, #0xe0f	! ah = request, al = mask for nybble
	mov	al, #0xf
	and	al, dl
	add	al, #0x90	! convert al to ascii hex (four instructions)
	daa
	adc	al, #0x40
	daa
!	int	0x10
	call	print_chr
	loop	print_digit
	ret
print_chr:
!	mov	bl,#7
	mov	bx,#7		! for older BIOS, bh must be current page
	mov	ah,#0xe
	int	0x10
	ret

/*
 * This procedure turns off the floppy drive motor, so
 * that we enter the kernel in a known state, and
 * don't have to worry about it later.
 */
kill_motor:
	push dx
	mov dx,#0x3f2
	xor al, al
	outb
	pop dx
	ret

.org 0x1DD
sectors:
	.word 0

disksizes:
	.byte 36,18,15,9
disksizes_end:

.org 0x1E3
msg1:
	.byte 13,10,7
	.ascii "ELKS Boot"
msg1end:

.org 0x1EF
	.word SETUPSEG
setup_sects:
	.byte SETUPSECS
root_flags:
	.word CONFIG_ROOT_RDONLY
syssize:
	.word SYSSIZE
swap_dev:
	.word SWAP_DEV
ram_size:
	.word RAMDISK
vid_mode:
	.word SVGA_MODE
root_dev:
	.word ROOT_DEV
boot_flag:
	.word 0xAA55
.org 0x200
