
/*
!
! SYS_SIZE is the number of clicks (16 bytes) to be loaded.
! 0x7F00 is 0x7F000 bytes = 508kB, more than enough for current
! versions of linux which compress the kernel
!
*/

#include <linuxmt/config.h>
SYSSIZE = DEF_SYSSIZE

/*
!
!	bootsect.s		Copyright (C) 1991, 1992 Linus Torvalds
!	modified by Drew Eckhardt
!	modified by Bruce Evans (bde)
!	modified by Alan Cox for Linux/ELKS and 8088 compatiblity
!	modified for Linux/16 by Chad Page 
!
! bootsect.s is loaded at 0x7c00 by the bios-startup routines, and moves
! itself out of the way to address INITSEG:0, and jumps there.
!
! bde - should not jump blindly, there may be systems with only 512K low
! memory.  Use int 0x12 to get the top of memory, etc.
!
! It then loads 'setup' directly after itself (INITSEG+20:0), and the system
! at SYSSEG:0, using BIOS interrupts. 
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
*/

.code16

.text

SETUPSECS = 4				/* nr of setup-sectors */
BOOTSEG   = 0x07C0			/* original address of boot-sector */
INITSEG   = DEF_INITSEG		/* we move boot here - out of the way */
SETUPSEG  = DEF_SETUPSEG
SYSSEG    = DEF_SYSSEG		/* initial kernel load address */
SYSSEGB   = DEF_SYSSEG + 2

// ROOT_DEV is now written by "build".
ROOT_DEV = 0

#ifndef SVGA_MODE
#define VGA_ASK 	0xfffd	/* ask for it at bootup */
#define SVGA_MODE	VGA_ASK
#endif

#ifndef RAMDISK
#define RAMDISK 	0
#endif 

#define DUMMYBOOT
#include <linuxmt/boot.h>

.global     _start
_start:
#if 0   /* hook for debugger, harmless unless BIOS is fussy (old HP) */
        int     3
#endif
	mov     $BOOTSEG,%ax
	mov     %ax,%ds
	mov     $INITSEG,%ax
	mov     %ax,%es
	mov     $256,%cx
	xor     %si,%si
	xor     %di,%di
	cld
	rep
	movsw
	ljmp    $INITSEG,$go

// ax and es already contain INITSEG

go:	mov     $0x4000-12,%di
/*
! 0x4000 is arbitrary value >= length of
! bootsect + length of setup + room for stack
! 12 is disk parm size

! bde - changed 0xff00 to 0x4000 to use debugger at 0x6400 up (bde).  We
! wouldn't have to worry about this if we checked the top of memory.  Also
! my BIOS can be configured to put the wini drive tables in high memory
! instead of in the vector table.  The old stack might have clobbered the
! drive table.
*/

	mov     %ax,%ds
	mov     %ax,%ss  /* put stack at INITSEG:0x4000-12. */
	mov     %di,%sp
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

	mov     $0x78,%bx
// 0:bx is parameter table address
    push    %ds
    push    %es
    xor     %ax,%ax
    mov     %ax,%es
    lds     %es:(%bx),%si
    pop     %es

// ds:si is source

	mov     $6,%cl
// copy 12 bytes
    cld
    push    %di

    rep
    movsw

	pop     %di
	pop     %ds
					/* what kind of instruction is this? */
    movb    $36,4(%di)  /* patch sector count */

	push    %ds
//	xor	ax,ax			ax still 0
	mov     %ax,%ds
	mov     %di,(%bx)
	mov	%es,2(%bx)
	pop	%ds

/*
! load the setup-sectors directly after the bootblock.
! Note that 'es' is already set up.
! Also cx is 0 from rep movsw above.
*/

load_setup:
	xor     %ah,%ah			/* reset FDC */
    xor     %dl,%dl
	int     $0x13

	xor     %dx,%dx			/* drive 0, head 0 */
	mov    	$0x0002,%cx		/* sector 2, track 0 */
	mov     $0x0200,%bx		/* address = 512, in INITSEG */
	mov     $0x02,%ah		/* service 2, nr of sectors */
	mov     setup_sects,%al	/* (assume all on head 0, track 0) */
	int     $0x13			/* read it */
	jnc     ok_load_setup	/* ok - continue */

	push    %ax			/* dump error code */
	call    print_nl
	mov     %sp,%bp
	call    print_hex
	pop     %ax

	jmp     load_setup

ok_load_setup:

// Get disk drive parameters, specifically nr of sectors/track

#if 0

/*
! bde - the Phoenix BIOS manual says function 0x08 only works for fixed
! disks.  It doesn't work for one of my BIOS's (1987 Award).  It was
! fatal not to check the error code.
*/

	xor	dl,dl
	mov	ah,#0x08		! AH=8 is get drive parameters
	int	$0x13
	xor	ch,ch
#else

/*
! It seems that there is no BIOS call to get the number of sectors.  Guess
! 36 sectors if sector 36 can be read, 18 sectors if sector 18 can be read,
! 15 if sector 15 can be read.  Otherwise guess 9.
*/

	mov     $disksizes,%si		/* table of sizes to try */

probe_loop:
	lodsb
	cbw				/* extend to word */
	mov     %ax,sectors
	cmp     $disksizes_end,%si
	jae     got_sectors		/* if all else fails, try 9 */
	xchg    %cx,%ax			/* cx = track and sector */
	xor     %dx,%dx			/* drive 0, head 0 */
	xor     %bl,%bl
	mov     setup_sects,%bh
	inc     %bh
	shl     $1,%bh			/* address after setup (es = cs) */
	mov     $0x0201,%ax		/* service 2, 1 sector */
	int     $0x13
	jc      probe_loop		/* try next value */

#endif

got_sectors:

/*
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
*/

	mov     $msg1end-msg1,%cx
	mov     $msg1,%si
nxt_chr:
	lodsb
	call    print_chr
	loop    nxt_chr

/*
! ok, we've written the message, now
! we want to load the system (at 0x10000)
*/

	mov     $SYSSEG,%ax
	mov     %ax,%es		/* segment of 0x010000 */
	call    read_it
	call    kill_motor
	mov     $':',%al
	call    print_chr
	call    print_nl

/*
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
*/
	ljmp    $SETUPSEG,$0

/*
! This routine loads the system at address 0x10000, making sure
! no 64kB boundaries are crossed. We try to load it as fast as
! possible, loading whole tracks whenever we can.
!
! in:	es - starting address segment (normally 0x1000)
!
*/
head:	.word 0			// current head
track:	.word 0			// current track

/*
!dx=sectors read of current track
!bx=offset in memory of block
!si=sectors read to block
!al=number to read
!
*/
read_it:
	mov     setup_sects,%dx
	inc     %dx
	mov     %es,%ax
	test    $0x0fff,%ax

die:
    jne     die			/* es must be at 64kB boundary */
	xor     %si,%si		/* zero sectors read */
	
rp_read:
	mov     %es,%ax
	sub     $SYSSEG,%ax
	cmp     syssize,%ax		/* have we loaded all yet? */
	jbe     ok1_read
	ret

ok1_read:
	mov     sectors,%ax		/* nr of sectors/track */

	sub     %dx,%ax		/* remaining of track */

	mov     %ax,%cx		/* cx= remaining */

	add     %si,%cx		/* boundary check */
	cmp     $128,%cl
	jbe     ok2_read		/* to much-> fill block */

	mov     $128,%ax		/* ax=0 */
	sub     %si,%ax		/* so much may be read */

ok2_read:
	mov     %si,%bx
	mov     $9,%cx
	shl     %cl,%bx
	call    read_track		/* do it */
	mov     %ax,%cx		/* cl=read blocks */

	add     %dx,%ax		/* %ax=new sectors */

	cmp     sectors,%ax		/* track done? */
	jne     ok3_read
	mov     $1,%ax		/* yes */
	sub     head,%ax
	jne     ok4_read		/* next head */
	incw    track		/* next track */
ok4_read:
	mov     %ax,head
	xor     %ax,%ax
ok3_read:
	mov     %ax,%dx

	add     %cx,%si
	cmp     $128,%si
	jne     rp_read

	mov     %es,%ax
	add     $0x10,%ah
	mov     %ax,%es

	xor     %si,%si
	jmp     rp_read

read_track:
	push    %dx
	push    %ax
	push    %bx
	
	mov     $'.',%al 	/* loading... message 2e = . */
	call    print_chr

	pop     %bx
	pop     %ax
	push    %ax
	push    %bx

	mov     %dx,%cx
	mov     track,%dx
	inc     %cx
	mov     %dl,%ch
	mov     head,%dx
	mov     %dl,%dh
	and     $0x0100,%dx
	mov     $2,%ah

	push    %dx				/* save for error dump */
	push    %cx
	push    %bx
	push    %ax

	int     $0x13

/*
!ah=02h al=nr sectors to read
!ch=cylinder
!cl=sector
!dh=head
!dl=drive
!es:bx=buffer
*/

	jc      bad_rt
	add     $8,%sp
	pop     %bx
	pop     %ax
	pop     %dx
	ret

bad_rt:
    push    %ax				/* save error code */
/*
!	push sectors
*/
	call    print_all			/* ah = error, al = read */
	
	
	xor     %ah,%ah
	xor     %dl,%dl
	int     $0x13
	

	add     $10,%sp
		
	pop     %bx
	pop     %ax
	pop     %dx
	jmp     read_track
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
	mov     $5,%cx		/* error code + 4 registers */
	mov     %sp,%bp

no_reg:
	push    %cx		/* save count left */
	add     $2,%bp		/* next register */
	call    print_hex	/* print it */
	pop     %cx
	loop    print_loop
	ret

print_loop:
	call    print_nl	/* nl for readability */

	
	mov     $0x0e05 + 'A' - 1,%ax
	sub     %cl,%al
	int     $0x10

	mov     $'X',%al
	int     $0x10

	mov     $':',%al
	int     $0x10
	jmp     no_reg

print_nl:
	mov     $0x0d,%al	/* CR */
	call    print_chr
	mov     $0x0a,%al	/* LF */
/*
!	int 	0x10
*/
	call    print_chr
	ret

/*
 *	print_hex is for debugging purposes, and prints the word
 *	pointed to by ss:bp in hexadecimal.
*/

print_hex:
/*
!	mov	bl,#7
*/
	mov     $4,%cx		/* 4 hex digits */
	mov     (%bp),%dx	/* load word into dx */

print_digit:
	push    %cx
	mov     $4,%cl
	rol     %cl,%dx		/* rotate so that lowest 4 bits are used */
	pop     %cx
/*
!	mov	ax, #0xe0f	! ah = request, al = mask for nybble
*/
	mov     $0xf,%al
	and     %dl,%al
	add     $0x90,%al	/* convert al to ascii hex (four instructions) */
	daa
	adc     $0x40,%al
	daa
/*
!	int	0x10
*/
	call    print_chr
	loop    print_digit
	ret

print_chr:
/*
!	mov	bl,#7
*/
	mov     $7,%bx		/* for older BIOS, bh must be current page */
	mov     $0xe,%ah
	int     $0x10
	ret

/*
 * This procedure turns off the floppy drive motor, so
 * that we enter the kernel in a known state, and
 * don't have to worry about it later.
 */
kill_motor:
	push    %dx
	mov     $0x03f2,%dx
	xor     %al,%al
	outb    %al,%dx
	pop     %dx
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
elks_magic:
	.ascii "ELKS Boot"
msg1end:

.org 0x1EF
	.word SETUPSEG
setup_sects:
	.byte SETUPSECS
root_flags:
	.word ROOTFLAGS
syssize:
	.word SYSSIZE
elks_flags:
	.byte EF_NONE
	.byte 0
ram_size:
	.word RAMDISK
vid_mode:
	.word SVGA_MODE
root_dev:
	.word ROOT_DEV
boot_flag:
	.word 0xAA55
.org 0x200
