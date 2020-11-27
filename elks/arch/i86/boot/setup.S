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
! changes to support .fartext headers and relocation
! Sep 2020 Greg Haerr
!
! The following data is passed to the main kernel (relative to INITSEG)
!
! index
!	...
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
!	...
!	0x2a:	size of the base memory, in kbytes, 2 bytes
!	0x30:	zero terminated string containing the processor's name, 16 bytes
!	...
!	0x50:	zero terminated string containing the cpuid, 13 bytes
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
!	0x1ff:	0xAA = boot_flag, High part
!
! NOTE! These had better be the same as in bootsect.s!
*/

#define __ASSEMBLY__
#include <linuxmt/config.h>
#include <linuxmt/boot.h>

// Relocating loader debug option
#define DEBUG_LOADER	0

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

	.arch i8086, nojumps
	.code16
	.text

	.global _start
_start:

#ifdef CONFIG_ROMCODE
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Entry point for non-EMU86 ROM systems

	.byte 0x55,0xaa		// sign for ROM-Extension
	.byte 2*CONFIG_ROM_CHECKSUM_SIZE // space for lengthcode (promsize/512)

	push	%ds
	xor	%ax,%ax		// DS = 0
	mov	%ax,%ds
	movw	$start_os,4*0x19 // set INT 19 os/loader vector
	mov	%cs,4*0x19+2
	pop	%ds 
	lret			// back to BIOS

#else


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Entry point for kernels loaded from DISK

// Check signature at end of setup
	mov	$SETUPSEG,%ax	// DS = setup CS
	mov	%ax,%ds
	cmpw	$SIG1,setup_sig1
	jne	no_sig
	cmpw	$SIG2,setup_sig2
	je	chk_blob

no_sig:	lea	no_sig_mess,%si
	call	puts
1:                             // And halt
	jmp	1b


// If setup and kernel were loaded as a blob, we need to separate them out,
// then move to our own stack

chk_blob:
	mov	$INITSEG,%ax
	mov	%ax,%ds
.if (EF_AS_BLOB & 0xff) == 0
	testb	$(EF_AS_BLOB>>8),elks_flags+1
.else
	testw	$EF_AS_BLOB,elks_flags
.endif
	jz	no_blob
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

done_move_kernel:
	cld
	mov	$INITSEG,%ax
	mov	%ax,%ss
	mov	$0x4000-12,%sp

no_blob:
#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Shared setup starts here for DISK and ROM cases
// Entry point for EMU86 ROM emulation

start_os:
	push	%cs
	pop	%ds
	mov	$hello_mess,%si
	call	puts

	mov	$INITSEG,%ax	// DS = setup data segment
	mov	%ax,%ds

#ifdef CONFIG_HW_KEYBOARD_BIOS
	mov	$0x0305,%ax	// set the keyboard repeat rate to the max
	xor	%bx,%bx
	int	$0x16
#endif

#ifdef CONFIG_HW_VGA
// check for EGA/VGA and some config parameters
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
#else
        movb  $0,15		// no VGA in system
#ifdef CONFIG_HW_VIDEO_LINES_PER_SCREEN
        mov   $CONFIG_HW_VIDEO_LINES_PER_SCREEN,%al
#else
        mov   $25,%al		// height of display in rows
#endif
#endif

novga:	mov	%al,14		// CGA 25 rows

// Get video-card data

	mov	$0x0f,%ah
	int	$0x10
	mov	%bx,4		// bh = display page
	mov	%ax,6		// al = video mode, ah = window width

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

	mov	$INITSEG,%ax
	mov	%ax,%ds
	int	$0x12		// determine the size of the basememory
	mov	%ax,0x2a

// End of shared setup code
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#ifdef CONFIG_ROMCODE
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Case for kernel in ROM
// Check for correct a.out header

#ifdef CONFIG_ROMFS_FS
	movw $0x0600,root_dev	// ROMFS is device 6,0
#else
	movw $0x0380,root_dev	// floppy is device 3,128
#endif
	mov $CONFIG_ROM_KERNEL_CODE,%ax	// DS = ROM a.out image
   	mov %ax,%ds

	cmpw $KERNEL_MAGICNUMBER,0 // 0 = hdr.magic
	jne aout_err

	cmpb $0x04,3		// 3 = hdr.a_cpu = 8086
	jne 1f
	jmp aout_ok

1:	cmpb $0x10,3		// 3 = hdr.a_cpu = 80386
	jne aout_err
	jmp aout_ok

aout_err:
   	push %cs	
   	pop %ds
   	lea msg_aout_err,%si
	call puts
err_loop:
	jmp err_loop


msg_aout_err:
	.byte 13,10
	.ascii "ERROR: Wrong kernel a.out format"
	.byte 0
msg_too_big:
	.byte 13,10
	.ascii "ERROR: kernel size > 64k"
	.byte 0

aout_ok:
	mov 10,%cx		// 10 = hiword hdr.a_text size
	jcxz 2f			// max 64k
size_error:
   	mov %cs,%ax
   	mov %ax,%ds
	lea msg_too_big,%si
	call puts
	jmp err_loop		// and halt

2:	cmp %cx,14		// 14 = hiword of hdr.a_data size
				// max 64k
				// CX = 0
	jnz size_error
	cmpb $0x20,4		// 4 = hdr.a_hdrlen
				// check for small 32 byte hdr
	jnz size_error		// error, stray fartext or relocations present

// Now copy kernel data segment to RAM
	mov 8,%ax		// 8 = hdr.a_text size
	mov %ax,%si		// Build up normalized pointer to point to
	and $0x000f,%si		// data contents to copy to RAM
				// SI = offset
	mov $4,%cl		// Compute segment
	ror %cl,%ax
	add $CONFIG_ROM_KERNEL_CODE+2,%ax
	mov 0x0c,%cx		// 12 = hdr.a_data size
	push %ds
	mov %ax,%ds		// DS:SI -> contents to copy
	mov $CONFIG_ROM_KERNEL_DATA,%ax	// ES = RAM kernel data segment
	mov %ax,%es
	xor %di,%di
	inc %cx
	shr $1,%cx		// copy words
	cld
	rep
	movsw
	pop %ds

// BX,CX,DX,SI,DI,DS,ES are expected in kernel crt0.S

	mov 16,%dx		// 16 = hdr.a_bss size
	mov 12,%si		// 12 = hdr.a_data size
	mov 8,%bx		//  8 = hdr.a_text size
	mov 20,%ax		// 20 = hdr.a_entry point
	mov %es,%cx		// ES = DS = kernel data segment
	mov %cx,%ds

#ifdef CONFIG_ROM_DEBUG
	int $3			// break for debugger just before kernel
#endif

	mov $CONFIG_ROM_KERNEL_CODE+2,%di // a.out + 32 = kernel CS
	push %di
	push %ax		// IP
	xor %di,%di		// far text size always 0
	lret			// jump to _start in crt0.S

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Case for normal kernel loaded from DISK and relocated
//
#else /* REL_SYS*/

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
	lret

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
	mov 4,%bx        // BX = header size
	cmp $0x20,%bx    // header size (no relocs)
	je sys_hdr_good
	cmp $0x40,%bx    // header size (relocs)
	je sys_hdr_good

sys_hdr_bad:
	mov %ss,%ax
	mov %ax,%ds
	lea sys_hdr_msg,%si
	call puts
1:                       // halt
	jmp 1b
sys_hdr_msg:
	.ascii "Bad system header!"
	.byte 0

// System header is good

sys_hdr_good:
	mov %sp,%bp
	mov 8,%dx     // -2(%bp) code size
	push %dx
	add $15,%dx   // align on next paragraph (issue #209)
	mov $4,%cl
	shr %cl,%dx   // DX = code segment size in paragraph
	shr %cl,%bx   // BX = header size in paragraphs

	mov 12,%ax    // -4(%bp) data size
	push %ax
	mov 16,%ax    // -6(%bp) bss size
	push %ax
	mov 20,%ax    // -8(%bp) entry point
	push %ax
	xor %ax,%ax   // -10(%bp) far text size
	push %ax
	push %ax      // -12(%bp) text reloc size
	push %ax      // -14(%bp) far text reloc size
	push %ax      // -16(%bp) data reloc size
	push %ax      // -18(%bp) kernel .text segment
	push %ax      // -20(%bp) kernel .fartext segment
	push %ax      // -22(%bp) kernel .data segment

// Relocate kernel code

	mov $'t',%ax     // display kernel .text segment
	call putc

	mov $SYSSEG,%ax  // start of a.out
	add %bx,%ax      // skip header
	mov %ax,%ds
	xor %si,%si

	mov $REL_SYSSEG,%ax
	mov %ax,%es
	mov %ax,-18(%bp) // save .text segment
	call hex4sp
	add %dx,%ax      // add code paragraphs
	mov %ax,-20(%bp) // save .data start in case no .fartext
	xor %di,%di

	mov -2(%bp),%cx  // code size in bytes
	cld
	rep
	movsb

// Relocate kernel far text

	cmp $0x04,%bx    // extended header?
	jne sys_no_far_text
	mov $SYSSEG,%ax  // readdress a.out header
	mov %ax,%ds
	mov 0x20,%ax     // text reloc size
	mov %ax,-12(%bp)
	mov 0x34,%ax     // far text reloc size
	mov %ax,-14(%bp)
	mov 0x24,%ax     // data reloc size
	mov %ax,-16(%bp)
	mov 0x30,%cx     // far text size
	mov %cx,-10(%bp)
	and %cx,%cx
	jz sys_no_far_text

	mov $'f',%ax      // display kernel .fartext segment
	call putc

	mov $SYSSEG,%ax
	add %bx,%ax       // skip header
	mov %ax,%ds
	xor %si,%si
	mov -2(%bp),%ax   // skip code bytes
	call add_ptr

	mov -18(%bp),%ax  // kernel .text segment
	add %dx,%ax       // skip code paragraphs
	mov %ax,%es
	mov %ax,-20(%bp)  // save .fartext segment
	xor %di,%di
	call hex4sp

	rep
	movsb
sys_no_far_text:

// Relocate kernel data (not bss)
// Kernel resets bss itself

	mov -10(%bp),%ax // conv far text size to paras
	mov $4,%cl
	shr %cl,%ax
	mov %ax,%cx

	mov $'d',%ax
	call putc

	mov $SYSSEG,%ax
	add %bx,%ax      // skip header
	mov %ax,%ds
	xor %si,%si
	mov -2(%bp),%ax  // skip code bytes
	call add_ptr
	mov -10(%bp),%ax // far text size
	call add_ptr
	push %si

	mov -20(%bp),%ax // kernel .fartext segment
	add %cx,%ax      // skip far text
	mov %ax,%es
	mov %ax,-22(%bp) // save .data segment
	xor %di,%di
	call hex4sp

	mov -4(%bp),%cx  // data size
	rep
	movsb

// Handle code/far text/data segment relocation

	pop %si          // get src ptr at fartext
	mov -4(%bp),%ax  // skip data size
	call add_ptr     // now at relocation entries

	mov -12(%bp),%cx // text reloc size
text_reloc:
	jcxz 1f
//	mov $'t',%ax
//	call putc
	mov -18(%bp),%ax // kernel .text segment
	mov %ax,%es

	call relocat
	sub $8,%cx
	add $8,%si
	jmp text_reloc
1:

	mov -14(%bp),%cx // far text reloc size
ftext_reloc:
	jcxz 2f
//	mov $'f',%ax
//	call putc
	mov -20(%bp),%ax // kernel .fartext segment
	mov %ax,%es

	call relocat
	sub $8,%cx
	add $8,%si
	jmp ftext_reloc
2:

	mov -16(%bp),%cx // data reloc size
data_reloc:
	jcxz 3f
//	mov $'d',%ax
//	call putc
	mov -22(%bp),%ax   // kernel .data segment
	mov %ax,%es

	call relocat
	sub $8,%cx
	add $8,%si
	jmp data_reloc
3:

	mov $'\r',%ax
	call putc
	mov $'\n',%ax
	call putc

// Load registers as kernel expects

	mov -22(%bp),%ax   // kernel .data segment
	mov %ax,%es
	mov %ax,%ds
	mov -2(%bp),%bx    // code size
	mov -4(%bp),%si    // data size
	mov -6(%bp),%dx    // bss size
	mov -8(%bp),%cx    // entry point
	mov -10(%bp),%di   // far text size

// Jump to kernel entry point

	mov -18(%bp),%ax   // kernel .text segment
	push %ax
	push %cx
	lret

// Relocate segment at ES: from relocation record at DS:SI
relocat:
	mov (%si),%di      // get r_vaddr
	mov 6(%si),%ax     // get r_type
	cmp $80,%ax        // R_SEGWORD
	jnz 9f
	mov 4(%si),%ax     // get r_symndx
	cmp $-2,%ax        // S_TEXT
	jnz 1f

	mov -18(%bp),%ax   // kernel .text segment
	jmp 3f
1:
	cmp $-5,%ax        // S_FTEXT
	jnz 2f

	mov -20(%bp),%ax   // kernel .fartext segment
	jmp 3f
2:
	cmp $-3,%ax        // S_DATA
	jnz 9f

	mov -22(%bp),%ax   // kernel .data segment
3:
#if DEBUG_LOADER
	push %ax           // display [seg:off=val]
	mov $'[',%ax
	call putc
	mov %es,%ax
	call hex4
	mov $':',%ax
	call putc
	mov %di,%ax
	call hex4
	mov $'=',%ax
	call putc
	pop %ax
	call hex4
	push %ax
	mov $']',%ax
	call putc
	pop %ax
#endif
	mov %ax,%es:(%di)
	ret
9:
	jmp sys_hdr_bad

// Add AX to DS:SI and normalize segment
add_ptr:
	push %cx
	add %si,%ax
	mov	%ax,%si
	and $15,%si
	mov $4,%cl
	shr %cl,%ax
	mov %ds,%cx
	add %cx,%ax
	mov %ax,%ds
	pop %cx
	ret
#endif /* REL_SYS*/

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

// Utility/debugging routines

// Write DS:SI asciiz string to console
1:	call	putc
puts:	lodsb
	test	%al,%al
	jnz	1b
	ret

// Write AX to console
putc:	push %bx
	push %cx
	push %dx
	mov $0x0E,%ah
	mov $7,%bx      // page 0
	int $0x10
	pop %dx
	pop %cx
	pop %bx
	ret

// Output hex nibble, byte and word. All registers saved.
hex1:	push %ax
	and $0x0F,%al
	add $'0',%al
	cmp $'9',%al
	jle 1f
	add $('A'-'9'-1),%al
1:	call putc
	pop %ax
	ret

hex2:	push %ax
	push %cx
	push %dx
	mov %al,%dl
	mov $4,%cl
	shr %cl,%al
	call hex1
	mov %dl,%al
	call hex1
	pop %dx
	pop %cx
	pop %ax
	ret

hex4:	push %ax
	push %ax
	mov %ah,%al
	call hex2
	pop %ax
	call hex2
	pop %ax
	ret

hex4sp: call hex4
	push %ax
	mov $' ',%ax
	call putc
	pop %ax
	ret

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
	.ascii "\r\nELKS Setup "
	.byte 0

// variables in ROM are not very useful
start_sys_seg:	.word	SYSSEG


// This must be last
setup_sig1:	.word	SIG1
setup_sig2:	.word	SIG2
