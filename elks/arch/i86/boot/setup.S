/*
!
!	setup.S		Copyright (C) 1991, 1992 Linus Torvalds
!
! setup.s is responsible for getting the system data from the BIOS,
! and putting them into the appropriate places in system memory.
! both setup.s and system has been loaded by the bootblock.
!
! This code asks the bios for memory/disk/other parameters, and
! puts them in a "safe" place: INITSEG:0-INITSEG:01FF, ie where the
! boot-block used to be. It is then up to the protected mode
! system to read them from there before the area is overwritten
! for buffer-blocks.
!
! Move PS/2 aux init code to psaux.c
! (troyer@saifr00.cfsat.Honeywell.COM) 03Oct92
!
! some changes and additional features by Christoph Niemann,
! March 1993/June 1994 (Christoph.Niemann@linux.org)
!
! changes for ROM-Version (ELKS) by Christian Mardmoller
! Juni / 1999 chm@kdt.de
!   This code is called after the BIOS-POST and replaces
!   the BIOS OS/loader
!
! The following data is passed to the main kernel (relative to INITSEG)
!
! index 0:	cursor position, 2 bytes
!	2:	extended memory size in K, 2 bytes
!	4:	display page
!	6:	video mode, 1 byte
!	7:	window width, 1 byte
!	8:	video data, 2 bytes
!	10:	mono/color, video memory size, 2 bytes 
!	12:	feature bits, switch settings, 2 bytes
!	14:	window height, 1 byte
!	15:	VGA present, 1 byte
!			0 = not present
!			1 = present
!	16	Point height of character matrix (taken from BDA), 2 bytes
!	0x20:	Processor type, 1 byte
!			0  = 8088
!			1  = 8086
!			2  = NEC V20
!			3  = NEC V30
!			4  = 80188
!			5  = 80186
!			6  = 80286
!			7  = 80386
!			8  = 80486
!			9  = Pentium
!			10 = Pentium PRO
!			255 = VM86 mode
!	0x21:	FPU type, 1 byte
!			0 = no fpu
!			1 = 8087
!			2 = 80287
!			3 = 80387 or above
!	0x22:	cpuid available -> 1, otherwise 0, 1 byte
!	0x23:	processor family, 1 byte
!	0x24:	mask model, 1 byte
!	0x25:	mask revision, 1 byte
!	0x26:	capability flag, 4 bytes
!	0x2a:	size of the base memory, in kytes, 2 bytes
!	0x30:	zero terminated string containing the processor's name, 16 bytes
!	0x40:	zero terminated string containing the fpu name, 16 bytes
!	0x50:	zero terminated string containing the cpuid, 13 bytes
!	0x80:	BIOS data for harddisk 0, 6 bytes
!			0x80, no. of heads, 2 bytes (0, no disk)
!			0x82, no. of sectors/track, 2 bytes
!			0x84, no. of cylinders - 1, 2 bytes
!	0x86:	BIOS data for harddisk 1, 6 bytes
!			0x86, no. of heads, 2 bytes (0, no disk)
!			0x88, no. of sectors/track, 2 bytes
!			0x8A, no. of cylinders - 1, 2 bytes
!	0x8c:	data for floppy disk 0, 6 bytes
!			0x8C, no. of heads, 2 bytes (0, no disk)
!			0x8E, no. of sectors/track, 2 bytes
!			0x90, no. of cylinders, 2 bytes
!	0x92:	data for floppy disk 1, 6 bytes
!			0x92, no. of heads, 2 bytes (0, no disk)
!			0x94, no. of sectors/track, 2 bytes
!			0x96, no. of cylinders, 2 bytes
!	...
!	0x1ef:	SETUPSEG
!	0x1f1:	SETUPSECS
!	0x1f2:	ROOTFLAGS
!	0x1f4:	SYSSIZE
!	0x1f6:	SWAP_DEV
!	0x1f8:	RAMDISK
!	0x1fa:	SVGA_MODE
!	0x1fc:	ROOT_DEV
!	0x1fe:	0x55 = boot_flag, Low part
!	0x1ff:	0xAA = boot_flag, High part/PS2 mouse device present (0, not present)
!
! NOTE! These had better be the same as in bootsect.s!
*/

#define __ASSEMBLY__
#include <linuxmt/config.h>

// Signature words to ensure LILO loaded us right
#define SIG1	0xAA55
#define SIG2	0x5A5A

#define MINIX_SPLITID_LOW 0x0301L
#define KERNEL_MAGICNUMBER MINIX_SPLITID_LOW

#ifndef CONFIG_ROMCODE
  INITSEG  = DEF_INITSEG	// (DATASEG) we move boot here - out of the way
  SYSSEG   = DEF_SYSSEG 	// system loaded at 0x10000 (65536).
  SETUPSEG = DEF_SETUPSEG	// this is the current code segment
#else
  INITSEG  = CONFIG_ROM_SETUP_DATA
  SYSSEG   = CONFIG_ROM_KERNEL_CODE
  SETUPSEG = CONFIG_ROM_SETUP_CODE
#endif

.code16

.text

.global start
start:
#ifdef CONFIG_ROMCODE
    .byte 0x55, 0xaa      //;sign for ROM-Extention
    .byte 0x04            //;space for lengthcode (promsize/512)

    push %ds

#else
// Bootlin depends on this being done early
	mov	$0x01500,%ax	//;Read disk type/DASD
	mov	$0x81,%dl
	int	$0x13
#endif


// Check signature at end of setup
	mov	$SETUPSEG,%ax      //;setup codesegment
	mov	%ax,%ds
	cmpw	$SIG1,setup_sig1
	jne	bad_sig
	cmpw	$SIG2,setup_sig2
	jne	bad_sig
	jmp	good_sig     //;why double jmp
	nop


// Routine to print asciiz-string at DS:SI
prts_1:	mov	$0x0007,%bx   //page 0
	mov	$0x0e,%ah
	int	$0x10

prtstr:	lodsb
	test	%al,%al
	jnz	prts_1
	ret



// We now have to find the rest of the setup code/data
// in ROM the code is complete

bad_sig:
#ifndef CONFIG_ROMCODE
	mov	$INITSEG,%ax     //;code setup
	mov	%ax,%ds
	xor	%bh,%bh
	mov	497,%bl		// get setup sects from boot sector (SETUPSECS)
	sub	$4,%bx		// LILO loads 4 sectors of setup
	mov	$8,%cl
	shl	%cl,%bx		// convert to words
	mov	%bx,%cx
	push	%cx		// This may not be needed - Chad.
	mov	$3,%cl
	shr	%cl,%bx		// convert to segment
	pop	%cx
	add	$SYSSEG,%bx
//	seg cs
	mov	%bx,%cs:start_sys_seg

// Move rest of setup code/data to here
	mov	$2048,%di	// four sectors loaded by LILO
	sub	%si,%si
	mov	$SETUPSEG,%ax
	mov	%ax,%es
	mov	$SYSSEG,%ax
	mov	%ax,%ds
	rep
	movsw

	mov	$SETUPSEG,%ax
	mov	%ax,%ds
	cmpw	$SIG1,setup_sig1
	jne	no_sig
	cmpw	$SIG2,setup_sig2
	je	good_sig
no_sig:
#endif

	lea	no_sig_mess,%si
	call	prtstr
no_sig_loop:			// And halt
	jmp	no_sig_loop


//;------------------------------------------------
good_sig:
#ifdef CONFIG_ROMCODE
	xor %ax,%ax
	mov %ax,%ds                      //;Interrupttable

	movw $start_os,4*0x19
	mov %cs,4*0x19+2      //;set os/loader int

	pop %ds 
	lret                           //;back to BIOS
#endif


//;-------------------------------------------------------
//;-- from here the real loader starts
//;-------------------------------------------------------
start_os:
	mov	$INITSEG,%ax       //;datasegment setup.S
	mov	%ax,%ds

//; The root device is obviously /dev/rom0
//; when setup starts in ROM / Flash memory

#ifdef CONFIG_ROMCODE
#ifdef CONFIG_ROMFS_FS
	movw $0x0600,0x01FC  /* rom-flash */
#else
	movw $0x0380,0x01FC  /* bioshd */
#endif
#endif

// Get memory size (extended mem, kB)

#ifdef CONFIG_XMS_SIZE
	movw	$CONFIG_XMS_SIZE,2
#else
	mov	$0x88,%ah
	int	$0x15
	mov	%ax,2
#endif

// set the keyboard repeat rate to the max

#ifdef CONFIG_HW_KEYBOARD_BIOS
	mov	$0x0305,%ax
	xor	%bx,%bx		// clear bx
	int	$0x16
#endif

// check for EGA/VGA and some config parameters

#ifdef CONFIG_HW_VGA
 	mov	$0x12,%ah	// Get video data
	mov	$0x10,%bl
	int	$0x10
	mov	%ax,8
	mov	%bx,10
	mov	%cx,12
	mov	$0x5019,%ax
	cmp	$0x10,%bl
	je	novga
	mov	$0x1a00,%ax	// Added check for EGA/VGA discrimination
	int	$0x10
	mov	%ax,%bx
	mov	$0x5019,%ax
	movb	$0,15		// by default, no VGA
	cmp	$0x1a,%bl	// 1a means VGA, anything else EGA or lower
	jne	novga
	movb	$1,15		// we've detected a VGA
//	call	chsvga
#else
        movb  $0,15           // no VGA in system
#ifdef CONFIG_HW_VIDEO_LINES_PER_SCREEN
        mov   $CONFIG_HW_VIDEO_LINES_PER_SCREEN,%al
#else
        mov   $0x19,%al          // height of display (0x19 == 25 rows)
#endif
#endif

novga:	mov	%al,14		// CGA 25 rows
	mov	$0x03,%ah	// read cursor pos
	xor	%bh,%bh		// clear bh
	int	$0x10		// save it in known place, con_init fetches
	mov	%dx,0	        // it from INITSEG.

// Get video-card data:
	mov	$0x0f,%ah
	int	$0x10
	mov	%bx,4		// bh = display page
	mov	%ax,6		// al = video mode, ah = window width

#ifdef CONFIG_HW_VIDEO_LINES_PER_CHARACTER
	movw	$CONFIG_HW_VIDEO_LINES_PER_CHARACTER,16
#else
	xor	%ax,%ax
	mov	%ax,%es		// Access BIOS DATA AREA
//	seg es
	mov	%es:0x485,%ax	// POINTS - Height of character matrix
	mov	%ax,16
#endif

// check for PS/2 pointing device

//	mov	ax,#INITSEG     //ds was not changed sinse good_gig
//	mov	ds,ax
	movb	$0,0x1ff	// default is no pointing device
	nop
#ifdef CONFIG_HW_PS2_MOUSE
	int	$0x11		// int 0x11: equipment determination
	test	$0x04,%al	// check if pointing device installed
	jz	no_psmouse
	movb	$0xaa,0x1ff	// device present
	nop
no_psmouse:
#endif

	call	getcpu
	push	%es		// clear BIOS data for harddisk 0/1, 12 bytes
	mov	$INITSEG,%ax	// and BIOS data for floppy disk 0/1, 12 bytes
	mov	%ax,%es
	mov	$24,%cx
	mov	$0x80,%di
	xor	%ax,%ax
	cld
	rep
	stosw
	pop	%es
#ifdef CONFIG_HW_HARD_DRIVE
	call	gethd		// Get geometry for harddisk 0/1.
#endif
#ifdef CONFIG_HW_FLOPPY_DRIVE
	call	getfloppy	// Get geometry for floppy disk 0/1.
#endif
	mov	$INITSEG,%ax
	mov	%ax,%ds
	int	$0x12		// determine the size of the basememory
	mov	%ax,0x2a

//----------------------------
#ifdef CONFIG_ROMCODE
// We must now copy the kernel to RAM (SYSSEG)

	mov %cs,%ax          //;string is in this codeseg
	mov %ax,%ds
	lea load_kernel,%si
	call prtstr

   	mov $CONFIG_ROM_KERNEL_CODE,%ax   //;the ROM image
   	mov %ax,%ds

// test, if correct a.out header

   	cmpw $KERNEL_MAGICNUMBER,0
   	jne aouterr

   	cmpb $0x04,3   //;prozessortype = 8086
   	jne test1
	jmp aout_ok

test1:
	cmpb $0x10,3   //;prozessortype = 80386
	jne aouterr
	jmp aout_ok

aouterr:
   	mov %cs,%ax
   	mov %ax,%ds
   	lea kernel_aouterr,%si
   	call prtstr
err_loop:
	jmp err_loop


load_kernel:    .ascii "Loading kernel image..."
                .byte 13,10
                .byte 0
kernel_aouterr: .byte 13,10
                .ascii "--> ERROR: wrong kernel a.out Format!"
                .byte 0
kernel_to_big:  .byte 13,10
                .ascii "--> ERROR: kernel size > 64k"
                .byte 0


aout_ok:
   	mov $CONFIG_ROM_KERNEL_DATA,%ax
   	mov %ax,%es

   	mov 10,%cx    //;hi of text size
   	or %cx,%cx       //;max 64k
   	jz size_ok
size_error:
   	mov %cs,%ax
   	mov %ax,%ds
   	lea kernel_to_big,%si
   	call prtstr
   	jmp err_loop  //;and halt

size_ok:
        mov 14,%cx	   //; Check data section size
        or %cx,%cx
        jnz size_error     //; .data section too big

//;and now copy the kerneldata
// WARNING: Next code fails if code_size+data_size+32 > 64K
   	mov 0x08,%si  //;code size
   	add 0x04,%si  //;+header size
        xor %di,%di
        mov 0x0c,%cx  //;data size
        shr $1,%cx      //;copy words
        cld
        rep
        movsw

//; now ist the data and code ad the right position
#endif

/*
!--------------------------------------------------------
! We setup ds, es, and ss now
!
!
!	For BCC generated code the rules are simple
!
!	ES=DS=SS. SP is at the top end of the segment, data at the bottom
!	CS = DS is allowed (code then is start of data) or split.
!

!+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
! Case for kernel in ROM
!
*/
#if defined(CONFIG_ROMCODE)

        mov 0x10,%dx  //;bbs size
        mov 0x0c,%si  //;data size
        mov 0x08,%bx  //;text size

        mov %es,%cx
        mov %cx,%ds

#ifdef CONFIG_ROM_DEBUG
	int     $3               // break for debugger just before kernel
#endif

	mov	$SYSSEG+2,%bp
	pushw	%bp
	pushw	0x18	//; entry point
	lretw

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Case for kernel in disk to be relocated
//
#elif defined(REL_SYS)

// Relocate setup data

	mov	$INITSEG,%ax
	mov	%ax,%ds
	mov	$REL_INITSEG,%ax
	mov	%ax,%es
	xor	%si,%si
	xor	%di,%di
	mov	$256,%cx
	cld
	rep
	movsw

// Assume kernel data+bss size > 5 Kbytes
// Calculate values needed

	mov	$SYSSEG,%ax
	mov	%ax,%ds

// Check system header

	cmpw $0x0301,0   // ELKS a.out magic
	jne sys_hdr_bad
	cmpw $0x0430,2   // i8086 - executable with separated I & D
	jne sys_hdr_bad
	cmpb $0x20,4     // header size (no relocs)
	jne sys_hdr_bad
	jmp sys_hdr_good

sys_hdr_bad:
	mov	$SETUPSEG,%ax
	mov	%ax,%ds
	lea sys_hdr_msg,%si
	call prtstr
sys_hdr_loop:
	jmp sys_hdr_loop
sys_hdr_msg:
	.ascii "Bad sys hdr!"
	.byte 0

// System header is good

sys_hdr_good:
	mov	24,%bx
	push	%bx
	mov	12,%bx
	mov	%bx,%dx
	push	%bx		// Data size
	add	16,%bx
	add $0xF,%bx     // align on next paragraph (issue #209)
	mov	$4,%cl
	shr	%cl,%bx
	add	$REL_SYSSEG,%bx	// BX = Relocated kernel code segment
	mov	8,%ax
	push	%ax		// Code size
	mov	$4,%cl
	shr	%cl,%ax
	mov	%ax,%cx
	add	%bx,%ax		// AX = Relocated kernel data segment
	add	$SYSSEG+2,%cx
	push	16		// Bss size
	push	%cx		// Kernel data segment
	add $0xF,%dx     // align on next paragraph (issue #209)
	mov	$4,%cl
	shr	%cl,%dx
	add	%ax,%dx		// DX = Rel rel segment
	mov	%sp,%bp


/*
! Variables in stack as follows:
! Offset from BP	variable
!	8		entry point
!	6		data size
!	4		code size
!	2		bss size
!	0		kernel data segment
*/

	cmp	$SYSSEG+2,%bx
	ja	move_back

// Move forward

	mov	%bx,%es		// Move code segment
	mov	$SYSSEG+2,%cx
	mov	%cx,%ds
	mov	4(%bp),%cx
	xor	%si,%si
	mov	%si,%di
	rep
	movsb
	mov	%ax,%es		// Move data segment
	mov	(%bp),%ds
	mov	6(%bp),%cx
	xor	%si,%si
	jmp	rel_rel

// Move backward

move_back:
	std
	mov	%ax,%es		// Move data segment
	mov	(%bp),%ds
	mov	6(%bp),%cx
	mov	%cx,%si
	dec	%si
	mov	%si,%di
	rep
	movsb
	mov	%bx,%es		// Move code segment
	mov	$SYSSEG+2,%cx
	mov	%cx,%ds
	mov	4(%bp),%cx
	mov	%cx,%si
	dec	%si
rel_rel:
	mov	%si,%di
	rep
	movsb

// Relocate relocation code to next to relocated kernel data
// segment because current setup code might be overwritten

	mov	%dx,%es
	mov	%cs,%cx
	mov	%cx,%ds
	mov	$cont,%si
	xor	%di,%di
	push	%es		// build far return address
	push	%di
	mov	$end_reloc-cont,%cx
	cld
	rep
	movsb
	lret			// jump to dx:0

// Still using the old stack. Switch to new stack
// AX = Rel. kernel data seg. BX = Rel. kernel code seg. DX = Rel. rel. seg.

cont:	mov	%ax,%ds
	pop	%di		// Empty old stack
	pop	%di
	pop	%ax
	pop	%si
	pop	%bp
	mov	%dx,%ss		// Set new stack
	mov	$end_reloc-cont+512,%sp
	mov	%di,%dx		// Move kernel data segment to REL_SYSSEG
	mov	$REL_SYSSEG,%cx
	mov	%cx,%es
	mov	%si,%cx
	push	%cx
	xor	%si,%si
	xor	%di,%di
	rep
	movsb

/*
! Load registers as expected by crt0.S
!	BX, Text size
!	SI, Data size
!	DX, BSS size
! DS,ES,CX, Kernel DS
*/

	xchg	%bx,%ax
	pop	%si
	mov	%es,%cx
	mov	%cx,%ds
	push	%ax		// Build far return address to relocated code segment
	push	%bp		// Address to start

	lret
end_reloc:
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Case for kernel in disk, no kernel relocation
//
#else

	mov $SYSSEG,%ax	// in ROM ds ist always set
	mov %ax,%ds	// Get the header into DS
	mov 8,%bx	// TSeg
	mov 12,%si	// DSeg
	mov 16,%dx	// BSeg
	mov 24,%bp	// start
	mov %bx,%ax	// Get Text size
	mov $4,%cl
	shr %cl,%ax	// Data in paragraphs

	mov $SYSSEG+2,%cx // Code starts here
	push %cx		// Build far return address to SYSSEG+2:start
	add %ax,%cx	// Segment base for data/bss/stack
	mov %cx,%ds
	mov %cx,%es
	push %bp
	retf
#endif
/*
!+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
!
! This routine checks that the keyboard command queue is empty
! (after emptying the output buffers)
!
! No timeout is used - if this hangs there is something wrong with
! the machine, and we probably couldn't proceed anyway.

! no call to this functions
*/
#if 0
empty_8042:
	call	delay
	in	al,#0x64	! 8042 status port
	test	al,#1		! output buffer?
	jz	no_output
	call	delay
	in	al,#0x60	! read it
	jmp	empty_8042
no_output:
	test	al,#2		! is input buffer full?
	jnz	empty_8042	! yes - loop
	ret
!
! Read a key and return the (US-)ascii code in al, scan code in ah
!
getkey:
	xor	ah,ah
	int	0x16
	ret

!
! Read a key with a timeout of 30 seconds. The cmos clock is used to get
! the time.
!
getkt:
	call	gettime
	add	al,#30		! wait 30 seconds
	cmp	al,#60
	jl	lminute
	sub	al,#60
lminute:
	mov	cl,al
again:	mov	ah,#0x01
	int	0x16
	jnz	getkey		! key pressed, so get it
	call	gettime
	cmp	al,cl
	jne	again
	mov	al,#0x20	! timeout, return default char `space'
	ret

!
! Flush the keyboard buffer
!
flush:	mov	ah,#0x01
	int	0x16
	jz	empty
	xor	ah,ah
	int	0x16
	jmp	flush
empty:	ret

!
! Read the cmos clock. Return the seconds in al
!

gettime:
	push	cx
	mov	ah,#0x02
	int	0x1a
	mov	al,dh			! dh contains the seconds
	and	al,#0x0f
	mov	ah,dh
	mov	cl,#0x04
	shr	ah,cl
	aad
	pop	cx
	ret

!
! Delay is needed after doing i/o
!
delay:
	.word	0x00eb			! jmp $+2
	ret
#endif
//;from never calls

//form this position are calles code

/*
!
! Probe for the CPU/Coprocessor
! These information is taken from "PC intern 3.0", Data Becker Verlag, 1992
! and from the Linux-Kernel, arch/i386/kernel/head.S
!
*/
getcpu:
	mov	$SETUPSEG,%ax         /* Codesegment setup.S */
	mov	%ax,%ds
#ifndef CONFIG_ROMCODE
	pushf
	xor	%ax,%ax
	push	%ax
	popf
	pushf
	pop	%ax
	popf
	and	$0xf000,%ax
	cmp	$0xf000,%ax
	je	is8086
	mov	$0x7000,%ax
	pushf
	push	%ax
	popf
	pushf
	pop	%ax
	popf
	and	$0x7000,%ax
	je	is80286
//
// Default & unknown CPU
//
	mov	$0xff,%cl
	lea	px86,%si
	jmp	getfpu
	nop
#endif

#if !defined(CONFIG_ROMCODE) || defined(CONFIG_CPU_8086)
is8086:
	mov	$0xff,%al
	mov	$0x21,%cl	// 80188/86 uses only the five lower
	shr	%cl,%al		// bits of cl to determine the number
	jnz	is80186		// of shifts.
	sti             
	xor	%si,%si
	mov	$0xffff,%cx
	nop
	rep
//	seg	es
	lodsb
	or	%cx,%cx
	jz	isv30
	call	queue
	jz	is8088
	mov	$1,%cl
	lea	p8086,%si
	jmp	getfpu
is8088:	xor	%cl,%cl
	lea	p8088,%si
	jmp	getfpu
is80186:call	queue
	jz	is80188
	mov	$5,%cl
	lea	p80186,%si
	jmp	getfpu
is80188:mov	$4,%cl
	lea	p80188,%si
	jmp	getfpu
isv30:	
	call	queue
	jz	isv20
	mov	$3,%cl
	lea	pv30,%si
	jmp	getfpu
isv20:	mov	$2,%cl
	lea	pv20,%si
	jmp     getfpu
#endif
#if !defined (CONFIG_ROMCODE) || defined(CONFIG_CPU_80286)
is80286:mov	$6,%cl
	lea	p80286,%si
//	jmp	getfpu
#endif

getfpu:
	//
	// Store the processor name and type
	//
	push	%cx
	mov	$INITSEG,%ax
	mov	%ax,%es
	mov	$0x30,%di
	mov	$16,%cx
	cld
con_cp1:
	lodsb
	stosb
	or	%al,%al
	loopnz	con_cp1
	mov	$0x50,%di
	lea	v_id,%si
	mov	$13,%cx
	rep
	movsb
	pop	%cx
	mov	$INITSEG,%ax
	mov	%ax,%ds
	mov	%cl,0x20

#ifdef CONFIG_HW_FPU
	fninit
	fnstcw	copro
	mov	copro,%ax
	cmp	$3,%ah
	jne	nofpu
	and	$0xff7f,copro
	fldcw	copro
	fdisi
	fstcw	copro
	test	$0x0080,copro
	jnz	is8087
	finit		// 80287/387 test
	fld1
	fldz
	fdiv	%st,%st(1)
	fld	%st
	fchs
	fcompp
	fstsw	copro
	mov	copro,%ax
	sahf
	jz	is80287
	mov	$3,%cl
	jmp	gotfpu
is80287: mov	$2,%cl
	jmp	gotfpu
is8087:	mov	$1,%cl
	jmp	gotfpu
#endif
nofpu:	xor	%cl,%cl

gotfpu:	mov	$INITSEG,%ax
	mov	%ax,%es
	mov	$SETUPSEG,%ax
	mov	%ax,%ds
//	seg	es
	mov	%cl,%es:0x21
	xor	%ch,%ch
	add	%cx,%cx
	lea	fpu_t,%si
	add	%cx,%si
	mov	(%si),%si
	mov	$0x40,%di
	mov	$16,%cx
	cld
con_cp2:
	lodsb
	stosb
	or	%al,%al
	loopnz	con_cp2
	ret

#if !defined(CONFIG_ROMCODE) || defined(CONFIG_CPU_8086)
/*
!
! Determine the length of the prefetch queue. 8088/188/v20 has
! a 4 bytes queue, 8086/186/v30 has 6 bytes.
!
! In ROM we can't change the code, we must copy to RAM
! Using Kernel dataseg
!
*/
queue:
#ifdef CONFIG_ROMCODE
	pushw	%ds
	movw	$CONFIG_ROM_KERNEL_DATA,%ax
	movw	%ax,%es
	movw	%cs,%ax
	movw	%ax,%ds
	movw	$queue_start,%si
	xorw	%di,%di
	movw	$0x20,%cx
	cli
	cld
	rep movsw
	sti
	xorw	%ax,%ax
	popw	%ds
	pushw	%es
	pushw	%ax
	lret
#endif
queue_start:

	mov	%cs,%ax
	mov	%ax,%es
	xor	%dx,%dx
	std
	lea	q_end,%di
#ifdef CONFIG_ROMCODE
        sub $queue_start,%di   //;we have new offset
#endif	
	mov	$0xfb,%al
	mov	$0x03,%cx
	cli
	rep
	stosb
	cld
	nop
	nop
	nop
	inc	%dx
q_end:	nop
	sti

#ifdef CONFIG_ROMCODE
        ljmp $CONFIG_ROM_SETUP_CODE,$queue_end
queue_end:
#endif

	or	%dx,%dx
	ret
#endif

/*
!
! Determine the number and type of floppy disks
! attached to our system.
!
*/
#ifdef CONFIG_HW_FLOPPY_DRIVE
getfloppy:
	mov	$INITSEG,%ax
	mov	%ax,%es
	mov	%ax,%ds
	mov	0x20,%bl	// Get processor type
	mov	$SETUPSEG,%ax
	mov	%ax,%ds
	int	$0x11		// only ax is changed by int 0x11
	test	$1,%al		// bit 0 set -> floppy present
	jz	no_floppy
	cmp	$5,%bl
	jle	is_xt
/*
!
! AT architecture. The BIOS tells us the number and capacity of the
! available floppy disks.
!
*/
	xor	%dl,%dl
	mov	$0x08,%ah
	push	%es
	int	$0x13		// changes es
	pop	%es
	jc	no_floppy	// c-flag is set if operation fails
	or	%bl,%bl		// the drive code is returned in bl
	jz	no_floppy	// it has to be in the range 1..6
	cmp	$6,%bl
	ja	no_floppy
	xor	%bh,%bh
	dec	%bl
	add	%bl,%bl
	mov	floppies(%bx),%si
	mov	$0x8c,%di
	mov	$3,%cx
	rep
	movsw
	int	$0x11		// check for second floppy
	test	$0xc0,%al	// Bit 6,7
	jz	no_floppy
	mov	$0x08,%ah
	mov	$0x01,%dl
	push	%es
	int	$0x13		// changes es
	pop	%es
	jc	no_floppy	// c-flag is set if operation fails
	or	%bl,%bl		// the drive code is returned in bl
	jz	no_floppy	// it has to be in the range 1..6
	cmp	$6,%bl
	ja	no_floppy
	xor	%bh,%bh
	dec	%bl
	add	%bl,%bl
	mov	floppies(%bx),%si
	mov	$0x8c+6,%di
	mov	$3,%cx
	rep
	movsw
	ret
is_xt:
/*
!
! XT architecture. Ask the BIOS about the number of available floppy
! disks and assume that they have a capacity of 360 KB.
!
! ax contains the result of int 0x11 when jumped here!
!
*/
	int	$0x11
	mov	$3,%cx
	mov	$0x8c,%di
	lea	f360,%si
	rep
	movsw
	test	$0xc0,%al	// Bit 6,7
	jz	no_floppy	// second floppy detected
	mov	$3,%cx
	lea	f360,%si
	rep
	movsw
no_floppy: ret

f360:	.word 2,9,40
f720:	.word 2,9,80
f1200:	.word 2,15,80
f1440:	.word 2,18,80
f2880:	.word 2,36,80
floppies: .word f360, f1200, f720, f1440, f2880, f2880
#endif

//
// gethd
//
#ifdef CONFIG_HW_HARD_DRIVE
gethd:
	mov	$INITSEG,%ax
	mov	%ax,%ds
	mov	%ax,%es






	mov	$0x08,%ah	// check for first drive
	mov	$0x80,%dl
	int	$0x13
	jc	no_hd0		// carry flag set -> an error occured
	or	%ah,%ah
	jnz	no_hd0		// error code != 0 -> bad
	or	%dl,%dl
	jz	no_hd0		// dl contains the number of harddisks
	push	%dx
	mov	%dh,%dl
	xor	%dh,%dh
	inc	%dx		// DX -> no. of heads
	mov	%cx,%bx
	and	$0x3f,%bx	// BX -> no. of sectors
	mov	%ch,%al
	mov	%cl,%ah
	mov	$6,%cl
	shr	%cl,%ah		// AX -> no. of cylinders - 1
	mov	%dx,0x80
	mov	%bx,0x82
	mov	%ax,0x84
	pop	%dx
	dec	%dl		// only one harddisk?
	jz	no_hd1
no_hd0:






	mov	$0x08,%ah	// check for second drive
	mov	$0x81,%dl
	int	$0x13
	jc	no_hd1
	or	%ah,%ah
	jnz	no_hd1
	mov	%dh,%dl
	xor	%dh,%dh
	inc	%dx		// DX -> no. of heads
	mov	%cx,%bx
	and	$0x3f,%bx	// BX -> no. of sectors
	mov	%ch,%al
	mov	%cl,%ah
	mov	$6,%cl
	shr	%cl,%ah		// AX -> no. of cylinders - 1
	mov	%dx,0x86
	mov	%bx,0x88
	mov	%ax,0x8a
no_hd1:	ret
#endif


//
// The processor name must not be longer than 15 characters!
//
#if !defined(CONFIG_ROMCODE) || defined(CONFIG_CPU_8086)
p8088:	.ascii "Intel 8088"
	.byte 0
p8086:	.ascii "Intel 8086"
	.byte 0
pv20:	.ascii "NEC V20"
	.byte 0
pv30:	.ascii "NEC V30"
	.byte 0
p80188:	.ascii "Intel 80188"
	.byte 0
p80186:	.ascii "Intel 80186"
	.byte 0
#endif
#if !defined(CONFIG_ROMCODE) || defined(CONFIG_CPU_80286)
p80286:	.ascii "Intel 80286"
	.byte 0
#endif
#if !defined(CONFIG_ROMCODE)
px86:   .ascii "Unknown x86"
	.byte 0
#endif
//
// Here is the CPU id stored
//
v_id:	.byte 0,0,0,0
v_id2:	.byte 0,0,0,0
v_id3:	.byte 0,0,0,0
	.byte 0
//
// FPU names. must be not longer than 15 characters!
//
f_none:	.ascii "no fpu"
	.byte 0
#ifdef CONFIG_HW_FPU
f8087:	.ascii "8087"
	.byte 0
f80287:	.ascii "80287"
	.byte 0
f80387: .ascii "80387 or above"
	.byte 0
#endif
fpu_t:	.word f_none
#ifdef CONFIG_HW_FPU
         .word f8087, f80287, f80387
#endif

#ifdef CONFIG_HW_FPU
copro:	.word 0
#endif

no_sig_mess:	.ascii	"No ELKS setup signature found ..."
		.byte	0x00

// variables in ROM are not very usefull
start_sys_seg:	.word	SYSSEG


// This must be last
setup_sig1:	.word	SIG1
setup_sig2:	.word	SIG2
