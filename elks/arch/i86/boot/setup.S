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
! changes to support having setup + ELKS kernel as one single blob
! March 2020 https://github.com/tkchia
!
! The following data is passed to the main kernel (relative to INITSEG)
!
! index
!	0:	cursor position, 2 bytes
!	2:	unused (was extended memory size), 2 bytes
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
!	0x21:	unused (was FPU type), 1 byte
!	0x22:	cpuid available -> 1, otherwise 0, 1 byte
!	0x23:	processor family, 1 byte
!	0x24:	mask model, 1 byte
!	0x25:	mask revision, 1 byte
!	0x26:	capability flag, 4 bytes
!	0x2a:	size of the base memory, in kytes, 2 bytes
!	0x30:	zero terminated string containing the processor's name, 16 bytes
!	0x40:	unused (was FPU name), 16 bytes
!	0x50:	zero terminated string containing the cpuid, 13 bytes
!	...
!	0x8c:	data for floppy disk 0, 6 bytes
!			0x8C, no. of heads, 2 bytes (0, no disk)
!			0x8E, no. of sectors/track, 2 bytes
!			0x90, no. of cylinders, 2 bytes
!	0x92:	data for floppy disk 1, 6 bytes
!			0x92, no. of heads, 2 bytes (0, no disk)
!			0x94, no. of sectors/track, 2 bytes
!			0x96, no. of cylinders, 2 bytes
!	...
!	0x1e2:	part_offset, 4 bytes. Sector offset of booted MBR partition
!	0x1e6:	elks_magic, 2 bytes
!	0x1ef:	SETUPSEG
!	0x1f1:	SETUPSECS, 1 byte
!	0x1f2:	ROOTFLAGS, 2 bytes
!	0x1f4:	SYSSIZE, 2 bytes
!	0x1f6:	ELKSFLAGS, 2 bytes
!	0x1f8:	RAMDISK
!	0x1fa:	SVGA_MODE
!	0x1fc:	ROOT_DEV, 2 bytes. Either BIOS boot device or actual kdev_t ROOT_DEV
!	0x1fe:	0x55 = boot_flag, Low part
!	0x1ff:	0xAA = boot_flag, High part/PS2 mouse device present (0, not present)
!
! NOTE! These had better be the same as in bootsect.s!
*/

#define __ASSEMBLY__
#include <linuxmt/config.h>
#include <linuxmt/boot.h>

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

.global entry
entry:
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
	jne	no_sig
	cmpw	$SIG2,setup_sig2
	jne	no_sig
	jmp	move_kernel     //;why double jmp
	nop


// Routine to print asciiz-string at DS:SI
prts_1:	mov	$0x0007,%bx   //page 0
	mov	$0x0e,%ah
	int	$0x10

prtstr:	lodsb
	test	%al,%al
	jnz	prts_1
	ret



// If setup and kernel were loaded as a blob, we need to separate them out,
// then move to our own stack

move_kernel:
#ifndef CONFIG_ROMCODE
	mov	$INITSEG,%ax
	mov	%ax,%ds
.if (EF_AS_BLOB & 0xff) == 0
	testb	$(EF_AS_BLOB>>8),elks_flags+1
.else
	testw	$EF_AS_BLOB,elks_flags
.endif
	jz	good_sig
	std			// move backwards --- we are moving from a
				// lower address to a higher one
	mov	setup_sects,%al	// find start of a.out kernel
	xor	%ah,%ah
	mov	$5,%cl
	shl	%cl,%ax
	add	$SETUPSEG,%ax
	mov	syssize,%bp
	mov	%bp,%cx		// first move the last sub-64KiB piece in place
	and	$0x0fff,%cx
	xor	%cx,%bp
	mov	%ax,%dx
	add	%bp,%dx
	mov	%dx,%ds
	lea	SYSSEG(%bp),%dx
	mov	%dx,%es
	jcxz	move_kernel_in_64ks
	shl	%cx
	shl	%cx
	shl	%cx
	mov	%cx,%si
	dec	%si
	shl	%si
	mov	%si,%di
	rep
	movsw
move_kernel_in_64ks:
	mov	%es,%ax
	cmp	$SYSSEG,%ax
	jz	done_move_kernel
	sub	$0x1000,%ax
	mov	%ax,%es
	mov	%ds,%ax
	sub	$0x1000,%ax
	mov	%ax,%ds
	mov	$0x80,%ch
	mov	$0xfffe,%si
	mov	%si,%di
	rep
	movsw
	jmp	move_kernel_in_64ks
#endif

no_sig:
	lea	no_sig_mess,%si
	call	prtstr
no_sig_loop:			// And halt
	jmp	no_sig_loop


//;------------------------------------------------
#ifndef CONFIG_ROMCODE
done_move_kernel:
	cld
	mov	$INITSEG,%ax
	mov	%ax,%ss
	mov	$0x4000-12,%sp
#endif

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

	push	%cs
	pop	%ds
	mov $hello_mess,%si
	call prtstr

	mov $INITSEG,%ax
	mov	%ax,%ds

//; The root device is obviously /dev/rom0
//; when setup starts in ROM / Flash memory

#ifdef CONFIG_ROMCODE
#ifdef CONFIG_ROMFS_FS
	movw $0x0600,root_dev  /* rom-flash */
#else
	movw $0x0380,root_dev  /* bioshd */
#endif
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
	mov	%es:0x485,%ax	// POINTS - Height of character matrix
	mov	%ax,16
#endif

// check for PS/2 pointing device

//	mov	ax,#INITSEG     //ds was not changed sinse good_gig
//	mov	ds,ax
	movb	$0,boot_flag+1	// default is no pointing device
	nop
#ifdef CONFIG_HW_PS2_MOUSE
	int	$0x11		// int 0x11: equipment determination
	test	$0x04,%al	// check if pointing device installed
	jz	no_psmouse
	movb	$0xaa,boot_flag+1 // device present
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

	cmpb $0x04,3   // processor type = 8086
	jne test1
	jmp aout_ok

test1:
	cmpb $0x10,3   // processor type = 80386
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

// BX,CX,DX,SI,DS,ES are expected in kernel crt0.S

	mov 0x10,%dx  // bss size
	mov 0x0c,%si  // data size
	mov 0x08,%bx  // text size
	mov 0x18,%ax  // entry point
	mov %es,%cx   // data segment
	mov %cx,%ds

#ifdef CONFIG_ROM_DEBUG
	int     $3               // break for debugger just before kernel
#endif

	mov $SYSSEG+2,%di
	push %di
	push %ax
	lret

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Case for kernel in disk to be relocated
//
#elif defined(REL_SYS)

	mov $INITSEG,%ax
	mov %ax,%ds

// Get the memory size
// TODO: optional BIOS INT 12h

	int $0x12   // AX in KB
	mov $6,%cl  // to paragraphs
	shl %cl,%ax

// Move to the highest & aligned 64K

	sub $0x1000,%ax
	and $0xF000,%ax
	mov %ax,%es

	mov 0x1F1,%al  // setup sector count
	cbw
	mov $8,%cl     // to words
	shl %cl,%ax
	mov %ax,%cx

	mov $512,%si   // skip raw sector
	xor %di,%di
	cld
	rep
	movsw

// Rebase CS DS ES SS to work in the 64K segment

	mov %es,%ax
	mov %ax,%ss   // automatic CLI for next instruction
	xor %sp,%sp

	push %ax
	mov $_next1,%ax
	push %ax
	retf

_next1:

// Relocate setup data (= raw boot sector)

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

// Check system header

	mov	$SYSSEG,%ax
	mov	%ax,%ds

	cmpw $0x0301,0   // ELKS a.out magic
	jne sys_hdr_bad
	cmpw $0x0430,2   // i8086 - executable with separated I & D
	jne sys_hdr_bad
	cmpb $0x20,4     // header size (no relocs)
	jne sys_hdr_bad
	jmp sys_hdr_good

sys_hdr_bad:
	mov %ss,%ax
	mov	%ax,%ds
	lea sys_hdr_msg,%si
	call prtstr
sys_hdr_loop:
	jmp sys_hdr_loop
sys_hdr_msg:
	.ascii "Bad system header!"
	.byte 0

// System header is good

sys_hdr_good:

	mov %sp,%bp
	mov 8,%dx     // code size
	push %dx
	add $0xF,%dx     // align on next paragraph (issue #209)
	mov	$4,%cl
	shr %cl,%dx   // DX = code segment size in paragraph

	mov 12,%ax    // data size
	push %ax
	mov 16,%ax    // bss size
	push %ax
	mov 20,%ax    // entry point
	push %ax

// Relocate kernel code

	mov $SYSSEG+2,%ax
	mov %ax,%ds
	mov $REL_SYSSEG,%ax
	mov %ax,%es

	mov -2(%bp),%cx  // code size
	xor	%si,%si
	mov	%si,%di
	cld
	rep
	movsb

// Relocate kernel data (not bss)
// Kernel resets bss itself

	mov $SYSSEG+2,%ax
	add %dx,%ax
	mov %ax,%ds
	mov $REL_SYSSEG,%ax
	add %dx,%ax
	mov %ax,%es

	mov -4(%bp),%cx  // data size
	xor %si,%si
	mov %si,%di
	cld
	rep
	movsb

// Load registers as kernel expects

	mov %es,%ax
	mov %ax,%ds

	mov -2(%bp),%bx    // code size
	mov -4(%bp),%si    // data size
	mov -6(%bp),%dx    // bss size
	mov -8(%bp),%cx    // entry point

// Jump to kernel entry point

	mov $REL_SYSSEG,%ax
	push %ax
	push %cx
	lret

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Case for kernel in disk, no kernel relocation
//
#else

	mov $SYSSEG,%ax	// in ROM ds ist always set
	mov %ax,%ds	// Get the header into DS
	mov 8,%bx	// TSeg
	mov 12,%si	// DSeg
	mov 16,%dx	// BSeg
	mov 20,%bp	// entry point
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

#endif /* REL_SYS */

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
! TODO: move probing to boot tools
! Probe for the CPU
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
	jmp	cpu_store
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
	jmp	cpu_store
is8088:	xor	%cl,%cl
	lea	p8088,%si
	jmp	cpu_store
is80186:call	queue
	jz	is80188
	mov	$5,%cl
	lea	p80186,%si
	jmp	cpu_store
is80188:mov	$4,%cl
	lea	p80188,%si
	jmp	cpu_store
isv30:	
	call	queue
	jz	isv20
	mov	$3,%cl
	lea	pv30,%si
	jmp	cpu_store
isv20:	mov	$2,%cl
	lea	pv20,%si
	jmp     cpu_store
#endif
#if !defined (CONFIG_ROMCODE) || defined(CONFIG_CPU_80286)
is80286:mov	$6,%cl
	lea	p80286,%si
//	jmp	cpu_store
#endif

cpu_store:
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

no_sig_mess:	.ascii	"No ELKS setup signature found ..."
		.byte	0x00

hello_mess:
	.ascii "\r\nELKS Setup\r\n"
	.byte 0

// variables in ROM are not very usefull
start_sys_seg:	.word	SYSSEG


// This must be last
setup_sig1:	.word	SIG1
setup_sig2:	.word	SIG2
