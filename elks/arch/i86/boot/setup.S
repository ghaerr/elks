!
!	setup.S		Copyright (C) 1991, 1992 Linus Torvalds
!
! setup.s is responsible for getting the system data from the BIOS,
! and putting them into the appropriate places in system memory.
! both setup.s and system has been loaded by the bootblock.
!
! This code asks the bios for memory/disk/other parameters, and
! puts them in a "safe" place: 0x90000-0x901FF, ie where the
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
! changes for ROM-Version (ELKS) by Christian Mardmˆller
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
!	8,10,12	video data
!	14: window height, 1 byte
!	16	video data
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
!	0x80:	BIOS data for harddisk 0/1, 12 bytes
!	0x8c:	data for floppy disk 0/1, 12 bytes

! NOTE! These had better be the same as in bootsect.s!

#define __ASSEMBLY__
#include <linuxmt/config.h>

! Signature words to ensure LILO loaded us right
#define SIG1	0xAA55
#define SIG2	0x5A5A

#define MINIX_SPLITID_LOW 0x0301L
#define KERNEL_MAGICNUMBER MINIX_SPLITID_LOW

#ifndef CONFIG_ROMCODE
  INITSEG  = DEF_INITSEG	! (DATASEG) we move boot here - out of the way
  SYSSEG   = DEF_SYSSEG 	! system loaded at 0x10000 (65536).
  SETUPSEG = DEF_SETUPSEG	! this is the current code segment
#else
  INITSEG  = CONFIG_ROM_SETUP_DATA
  SYSSEG   = CONFIG_ROM_KERNEL_CODE
  SETUPSEG = CONFIG_ROM_SETUP_CODE
#endif

.text

entry start
start:
#ifdef CONFIG_ROMCODE
    .byte 0x55, 0xaa      ;sign for ROM-Extention
    .byte 0x04            ;space for lengthcode (promsize/512)

    push ds

#else
! Bootlin depends on this being done early
	mov	ax,#0x01500
	mov	dl,#0x81
	int	0x13
#endif


! Check signature at end of setup
	mov	ax,#SETUPSEG      ;setup codesegment
	mov	ds,ax
	cmp	setup_sig1,#SIG1
	jne	bad_sig
	cmp	setup_sig2,#SIG2
	jne	bad_sig
	jmp	near good_sig     ;why double jmp


! Routine to print asciiz-string at DS:SI
prts_1:	mov	bx,#0x0007   !page 0
	mov	ah,#0x0e
	int	0x10

prtstr:	lodsb
	test	al,al
	jnz	prts_1
	ret



#ifndef CONFIG_ROMCODE
! We now have to find the rest of the setup code/data
! in ROM the code is complete

! variables in ROM are not very usefull
start_sys_seg:	.word	SYSSEG

bad_sig:
	mov	ax,#INITSEG     ;code setup
	mov	ds,ax
	xor	bh,bh
	mov	bl,[497]	! get setup sects from boot sector
	sub	bx,#4		! LILO loads 4 sectors of setup
	mov	cl,#8
	shl	bx,cl		! convert to words
	mov	cx,bx
	push	cx		! This may not be needed - Chad.
	mov	cl,#3
	shr	bx,cl		! convert to segment
	pop	cx
	add	bx,#SYSSEG
	seg cs
	mov	start_sys_seg,bx

! Move rest of setup code/data to here
	mov	di,#2048	! four sectors loaded by LILO
	sub	si,si
	mov	ax,#SETUPSEG
	mov	es,ax
	mov	ax,#SYSSEG
	mov	ds,ax
	rep
	movsw

	mov	ax,#SETUPSEG
	mov	ds,ax
	cmp	setup_sig1,#SIG1
	jne	no_sig
	cmp	setup_sig2,#SIG2
	jne	no_sig
	jmp	good_sig
#else
bad_sig:
#endif

no_sig:
	lea	si,no_sig_mess
	call	prtstr
no_sig_loop:			! And halt
	jmp	no_sig_loop


;------------------------------------------------
good_sig:
#ifdef CONFIG_ROMCODE
	xor ax,ax
	mov ds,ax                      ;Interrupttable

	mov word ptr[4*0x19],#start_os
	mov word ptr[4*0x19+2],cs      ;set os/loader int

	pop ds 
	retf                           ;back to BIOS
#endif




;-------------------------------------------------------
;-- from here the real loder starts
;-------------------------------------------------------
start_os:
	mov	ax,#INITSEG       ;datasegment setup.S
	mov	ds,ax


! Get memory size (extended mem, kB)

#ifdef CONFIG_XMS_SIZE
	mov	[2],#CONFIG_XMS_SIZE
#else
	mov	ah,#0x88
	int	0x15
	mov	[2],ax
#endif

! set the keyboard repeat rate to the max

#ifdef CONFIG_HW_KEYBOARD_BIOS
	mov	ax,#0x0305
	xor	bx,bx		! clear bx
	int	0x16
#endif

! check for EGA/VGA and some config parameters

#ifdef CONFIG_HW_VGA
 	mov	ah,#0x12
	mov	bl,#0x10
	int	0x10
	mov	[8],ax
	mov	[10],bx
	mov	[12],cx
	mov	ax,#0x5019
	cmp	bl,#0x10
	je	novga
	mov	ax,#0x1a00	! Added check for EGA/VGA discrimination
	int	0x10
	mov	bx,ax
	mov	ax,#0x5019
	movb	[15],#0		! by default, no VGA
	cmp	bl,#0x1a	! 1a means VGA, anything else EGA or lower
	jne	novga
	movb	[15],#1		! we've detected a VGA
!	call	chsvga
#else
        movb  [15],#0           ! no VGA in system
#ifdef CONFIG_HW_VIDEO_LINES_PER_SCREEN
        mov   al,#CONFIG_HW_VIDEO_LINES_PER_SCREEN
#else
        mov   al,#0x19          ! high of display (0x19 == 25 column)
#endif
#endif

novga:	mov	[14],al
	mov	ah,#0x03	! read cursor pos
	xor	bh,bh		! clear bh
	int	0x10		! save it in known place, con_init fetches
	mov	[0],dx	        ! it from 0x90000.

! Get video-card data:
	mov	ah,#0x0f
	int	0x10
	mov	[4],bx		! bh = display page
	mov	[6],ax		! al = video mode, ah = window width

#ifdef CONFIG_HW_VIDEO_LINES_PER_CHARACTER
        mov   [16],#CONFIG_HW_VIDEO_LINES_PER_CHARACTER
#else
	xor	ax,ax
	mov	es,ax		! Access low memory
	seg es
	mov	ax,[0x485]	! POINTS - Height of character matrix
	mov	[16],ax
#endif

! check for PS/2 pointing device

!	mov	ax,#INITSEG     !ds was not changed sinse good_gig
!	mov	ds,ax
	mov	[0x1ff],#0	! default is no pointing device
#ifdef CONFIG_HW_PS2_MOUSE
	int	0x11		! int 0x11: equipment determination
	test	al,#0x04	! check if pointing device installed
	jz	no_psmouse
	mov	[0x1ff],#0xaa	! device present
no_psmouse:
#endif

	call	getcpu
	push	es
	mov	ax,#INITSEG
	mov	es,ax
	mov	cx,#12
	mov	di,#0x80
	xor	ax,ax
	cld
	rep
	stosw
	pop	es
#ifdef CONFIG_HW_HARD_DRIVE
!	call	gethd
#endif
#ifdef CONFIG_HW_FLOPPY_DRIVE
	call	getfloppy
#endif
	mov	ax,#INITSEG
	mov	ds,ax
	int	0x12		! determine the size of the basememory
	mov	[0x2a],ax

!----------------------------
#ifdef CONFIG_ROMCODE
! We must now copy the kernel to RAM (SYSSEG)

        mov ax,cs          ;string is in this codeseg
	mov ds,ax
	lea si,load_kernel
	call prtstr

   	mov ax,#CONFIG_ROM_KERNEL_CODE   ;the ROM image
   	mov ds,ax

! test, if correct a.out header

   	cmp word ptr[0],#KERNEL_MAGICNUMBER
   	jne aouterr

   	cmp byte ptr [3],#0x04   ;prozessortype = 8086
   	jne test1
	jmp near aout_ok

test1:
	cmp byte ptr [3],#0x10   ;prozessortype = 80386
	jne aouterr
	jmp near aout_ok

aouterr:
   	mov ax,cs
   	mov ds,ax
   	lea si,kernel_aouterr
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
   	mov ax,#CONFIG_ROM_KERNEL_DATA
   	mov es,ax

   	mov cx,[10]    ;hi of size
   	or cx,cx       ;max 64k
   	jz size_ok
size_error:
   	mov ax,cs
   	mov ds,ax
   	lea si,kernel_to_big
   	call prtstr
   	jmp near err_loop  ;and halt

size_ok:
        mov cx,[14]
        or cx,cx
        jnz size_error     ;.data zu groﬂ

;and now copy the kerneldata
   	mov si,[0x08]  ;code size
   	add si,[0x04]  ;+header size
        xor di,di
        mov cx,[0x0c]  ;data size
        shr cx,1       ;copy words
        cld
        rep
        movsw

; now ist the data and code ad the right position

        mov dx,[0x10]  ;bbs size
        mov si,[0x0c]  ;data size
        mov bx,[0x08]  ;text size

        mov ax,#CONFIG_ROM_KERNEL_DATA
        mov ds,ax
        mov ss,ax    ;es is set
        mov sp,#0xfffe
#endif

!--------------------------------------------------------
! We setup ds, es, and ss now
!
!
!	For BCC generated code the rules are simple
!
!	ES=DS=SS. SP is at the top end of the segment, data at the bottom
!	CS = DS is allowed (code then is start of data) or split.
!

!header is only in ROM, ds points on it
#ifndef CONFIG_ROMCODE
	mov ax, #SYSSEG       ;in ROM ds ist always set
	mov ds, ax	! Get the header into DS
	mov ax, [8]	! TSeg
	mov bx,	ax	! Save Text size
	mov si, [12]	! DSeg
	mov dx, [16]	! BSeg
	mov cl, #4
	shr ax, cl	! Data in paragraphs

	mov cx, #SYSSEG+2 ! Code starts here
	add cx, ax	! Segment base for data/bss/stack
	mov ds, cx
	mov es, cx
!	mov ss, cx
!	mov sp, #0xFFFE	! Top of stack right at the end (temporary)
#endif

#ifdef CONFIG_ROMCODE
	push	dx	! BSS size
	push	si	! Data size
	push	bx	! Text size

 #ifdef CONFIG_ROM_DEBUG
        int 3               ;break for debugger
 #endif
        jmp far SYSSEG+2:3
#else
! changed jmpi 0,0x1002 to 3,0x1002 for kernel restart fix -AJB
	jmpi	0x00003,SYSSEG+2   !jmp offset 0 of segment 0x1002 (cs)
#endif

!------------------------------
! This routine checks that the keyboard command queue is empty
! (after emptying the output buffers)
!
! No timeout is used - if this hangs there is something wrong with
! the machine, and we probably couldn't proceed anyway.

! no call to this functions
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
;from never calls

!form this position are calles code

!
! Probe for the CPU/Coprocessor
! These information is taken from "PC intern 3.0", Data Becker Verlag, 1992
! and from the Linux-Kernel, arch/i386/kernel/head.S
!
getcpu:
	mov	ax,#SETUPSEG         /* Codesegment setup.S */
	mov	ds,ax
#ifndef CONFIG_ROMCODE
	pushf
	xor	ax,ax
	push	ax
	popf
	pushf
	pop	ax
	popf
	and	ax,#0xf000
	cmp	ax,#0xf000
	je	tmp86
	mov	ax,#0x7000
	pushf
	push	ax
	popf
	pushf
	pop	ax
	popf
	and	ax,#0x7000
	je	tmp286
!
! Check if the processor runs in VM-Mode (does not work with DOSEMU. Why not?)
!
	pushfd
	pop	eax
	and	eax,#0x020000
	jz	chk486
	mov	cl,#0xff
	lea	si,pvm86
	br	getfpu
!
! Probe for 486
!
chk486:	cli
	mov	ebx,esp
	and	esp,#0x0fffc
	pushfd
	pop	eax
	mov	ecx,eax
	xor	eax,#0x040000	! 1 << 18, AC flag
	push	eax
	popfd
	pushfd
	pop	eax
	push	ecx
	popfd
	xor	eax,ecx
	shr	eax,#18		! This can stay since it's 32-bit code :)
	and	eax,#1
	mov	esp,ebx
	sti
	mov	cl,#7
	add	cl,al
	or	al,al
	jnz	is486
#endif
#ifdef CONFIG_CPU_80386
        mov cl,#7
	lea	si,p80386
	br	getfpu
#endif
#ifndef CONFIG_ROMCODE
	lea	si,p80386
	br	getfpu
tmp86:	br	is8086
tmp286:	br	is80286
#endif

#if !defined(CONFIG_ROMCODE) || defined(CONFIG_CPU_80486)
is486:	! Now check whether this processor knows the CPU-id instruction
	!
	! This does not work. The code seems to destroy some other memory
	! areas. But why? Is it 16/32-bit stack corruption?
	!
	mov	ebp,esp
	and	esp,#0xfffc
	pushfd			! save flags
	pushfd
	pop	eax		! flags are in ax
	mov	ecx,eax
	xor	eax,#0x0200000	! 1 << 21, CPUID flag
	push	eax
	popfd
	pushfd
	pop	eax
	xor	ecx,eax
	popfd
	mov	esp,ebp
	and	eax,#0x0200000
	jz	told486
	xor	eax,eax
	db	0x0f, 0xa2	! cpuid
	mov	v_id,ebx
	mov	v_id2,edx
	mov	v_id3,ecx
	mov	eax,#1
	db	0x0f, 0xa2	! cpuid
	mov	cl,al
	and	ah,#0x0f	! cpu family
	and	al,#0xf0
	push	cx
	mov 	cl, #4
	shr	al,cl		! cpu model
	pop 	cx
	and	cl,#0x0f	! mask revision
	push	ds
	push	ax
	mov	ax,#INITSEG
	mov	ds,ax
	pop	ax
	mov	[0x23],ah	! x86 family
	mov	[0x24],al	! x86 model
	mov	[0x25],cl	! x86 mask revision
	mov	[0x26],edx	! cpu capability flag
	mov	dl,#1
	mov	[0x22],dl	! cpuid data valid
	pop	ds
!
! 486 processor with CPU-ID
!
	cmp	ah,#4		! 486 CPU
	jne	m_pentium
	lea	si,m486_t
	xor	ah,ah
	add	al,al		! 16 different 486 CPUs, no check needed
	add	si,ax
	mov	si,[si]
	mov	cl,#8
	br	getfpu
told486:jmp	old486
!
! Pentium or above
!
m_pentium:
	cmp	ah,#5
	jnz	m_ppro
	xor	ah,ah
	cmp	al,#3		! we currently know of 4 differrent Pentia
	jle	pen_ok
	mov	al,#4
pen_ok:	lea	si,m586_t
	add	al,al
	add	si,ax
	mov	si,[si]
	mov	cl,#9
	br	getfpu
!
! Pentium pro -- the best machine for ELKS :-)
!
m_ppro:	cmp	ah,#6
	jnz	m_unknown
	xor	ah,ah
	cmp	al,#1		! there a 2 known PPro versions
	jle	ppro_ok
	mov	al,#2
ppro_ok: lea	si,m686_t
	add	al,al
	add	si,ax
	mov	si,[si]
	mov	cl,#10
	jmp	getfpu
m_unknown:
	lea	si,m_x86
	mov	cl,#8
	jmp	getfpu
old486:	xor	cl,cl
	mov	ax,#INITSEG
	push	ds
	mov	ds,ax
	mov	[0x22],cl
	pop	ds
	mov	cl,#8
	lea	si,p80486
	jmp	getfpu
#endif

#if !defined(CONFIG_ROMCODE) || defined(CONFIG_CPU_8086)
is8086:
	mov	al,#0xff
	mov	cl,#0x21	! 80188/86 uses only the five lower
	shr	al,cl		! bits of cl to determine the number
	jnz	is80186		! of shifts.
	sti             
	xor	si,si
	mov	cx,#0xffff
	rep
	seg	es
	lodsb
	or	cx,cx
	jz	isv30
	call	queue
	jz	is8088
	mov	cl,#1
	lea	si,p8086
	jmp	getfpu
is8088:	xor	cl,cl
	lea	si,p8088
	jmp	getfpu
is80186:call	queue
	jz	is80188
	mov	cl,#5
	lea	si,p80186
	jmp	getfpu
is80188:mov	cl,#4
	lea	si,p80188
	jmp	getfpu
isv30:	
	call	queue
	jz	isv20
	mov	cl,#3
	lea	si,pv30
	jmp	getfpu
isv20:	mov	cl,#2
	lea	si,pv20
        jmp     getfpu
#endif
#if !defined (CONFIG_ROMCODE) || defined(CONFIG_CPU_80286)
is80286:mov	cl,#6
	lea	si,p80286
!	jmp	getfpu
#endif

getfpu:
	!
	! Store the processor name and type
	!
	push	cx
	mov	ax,#INITSEG
	mov	es,ax
	mov	di,#0x30
	mov	cx,#16
	cld
con_cp1:
	lodsb
	stosb
	or	al,al
	loopnz	con_cp1
	mov	di,#0x50
	lea	si,v_id
	mov	cx,#13
	rep
	movsb
	pop	cx
	mov	ax,#INITSEG
	mov	ds,ax
	mov	[0x20],cl

#ifdef CONFIG_HW_FPU
	fninit
	fnstcw	copro
	mov	ax,copro
	cmp	ah,#3
	jne	nofpu
	and	copro,#0xff7f
	fldcw	copro
	fdisi
	fstcw	copro
	test	copro,#0x0080
	jnz	is8087
	finit		! 80287/387 test
	fld1
	fldz
	fdiv	st,st(1)
	fld	st
	fchs
	fcompp
	fstsw	copro
	mov	ax,copro
	sahf
	jz	is80287
	mov	cl,#3
	jmp	gotfpu
is80287: mov	cl,#2
	jmp	gotfpu
is8087:	mov	cl,#1
	jmp	gotfpu
#endif
nofpu:	xor	cl,cl

gotfpu:	mov	ax,#INITSEG
	mov	es,ax
	mov	ax,#SETUPSEG
	mov	ds,ax
	seg	es
	mov	[0x21],cl
	xor	ch,ch
	add	cx,cx
	lea	si,fpu_t
	add	si,cx
	mov	si,[si]
	mov	di,#0x40
	mov	cx,#16
	cld
con_cp2:
	lodsb
	stosb
	or	al,al
	loopnz	con_cp2
	ret

#if !defined(CONFIG_ROMCODE) || defined(CONFIG_CPU_8086)
!
! Determine the length of the prefetch queue. 8088/188/v20 has
! a 4 bytes queue, 8086/186/v30 has 6 bytes.
!
! In ROM we can't change the code, we must copy to RAM
! Using Kernel dataseg
!
queue:
#ifdef CONFIG_ROMCODE
        push ds
        mov ax,#CONFIG_ROM_KERNEL_DATA
        mov es,ax
        mov ax,cs
        mov ds,ax
        mov si,#queue_start
        xor di,di
        mov cx,#0x20    
        cli
        cld
        rep
        movsw
        sti
        xor ax,ax
        pop ds
        push es
        push ax
        retf
#endif
queue_start:

	mov	ax,cs
	mov	es,ax
	xor	dx,dx
	std
	lea	di,q_end
#ifdef CONFIG_ROMCODE
        sub di,#queue_start   ;we have new offset
#endif	
	mov	al,#0xfb
	mov	cx,#0x03
	cli
	rep
	stosb
	cld
	nop
	nop
	nop
	inc	dx
q_end:	nop
	sti

#ifdef CONFIG_ROMCODE
        jmp far CONFIG_ROM_SETUP_CODE:queue_end
queue_end:
#endif

	or	dx,dx
	ret
#endif

!
! Determine the number and type of floppy disks
! attached to our system.
!
#ifdef CONFIG_HW_FLOPPY_DRIVE
getfloppy:
	mov	ax,#INITSEG
	mov	es,ax
	mov	ds,ax
	mov	bl,[0x20]
	mov	ax,#SETUPSEG
	mov	ds,ax
	int	0x11		! only ax is changed by int 0x11
	test	al,#1		! bit 0 set -> floppy present
	jz	no_floppy
	cmp	bl,#5
	jle	is_xt
!
! AT architecture. The BIOS tells us the number and capacity of the
! available floppy disks.
!
	xor	dl,dl
	mov	ah,#0x08
	push	es
	int	0x13		! changes es
	pop	es
	jc	no_floppy	! c-flag is set if operation fails
	or	bl,bl		! the drive code is returned in bl
	jz	no_floppy	! it has to be in the range 1..6
	cmp	bl,#6
	ja	no_floppy
	lea	si,floppies
	xor	bh,bh
	dec	bl
	add	bl,bl
	add	si,bx
	mov	si,[si]
	mov	di,#0x8c
	mov	cx,#3
	rep
	movsw
	int	0x11		! check for second floppy
	test	al,#0xc0	! Bit 6,7
	jz	no_floppy
	mov	ah,#0x08
	mov	dl,#0x01
	push	es
	int	0x13		! changes es
	pop	es
	jc	no_floppy	! c-flag is set if operation fails
	or	bl,bl		! the drive code is returned in bl
	jz	no_floppy	! it has to be in the range 1..6
	cmp	bl,#6
	ja	no_floppy
	lea	si,floppies
	xor	bh,bh
	dec	bl
	add	bl,bl
	add	si,bx
	mov	si,[si]
	mov	cx,#3
	mov	di,#(0x8c+6)
	rep
	movsw
	ret
is_xt:
!
! XT architecture. Ask the BIOS about the number of available floppy
! disks and assume that they have a capacity of 360 KB.
!
! ax contains the result of int 0x11 when jumped here!
!
	int	0x11
	mov	cx,#3
	mov	di,#0x8c
	lea	si,f360
	rep
	movsw
	test	al,#0xc0	! Bit 6,7
	jz	no_floppy	! second floppy detected
	mov	cx,#3
	lea	si,f360
	rep
	movsw
no_floppy: ret

f360:	dw 2,9,40
f720:	dw 2,9,80
f1200:	dw 2,15,80
f1440:	dw 2,18,80
f2880:	dw 2,36,80
floppies: dw f360, f1200, f720, f1440, f2880, f2880
#endif

!
! gethd
!
#ifdef CONFIG_HW_HARD_DRIVE
gethd:
	mov	ax,#INITSEG
	mov	ds,ax
	mov	es,ax
	mov	ah,#0x10
	mov	dl,#0x80
	int	0x13
	jc	no_hd0
	or	ah,ah
	jnz	no_hd0
	mov	ah,#0x08	! check for first drive
	mov	dl,#0x80
	int	0x13
	jc	no_hd0		! carry flag set -> an error occured
	or	ah,ah
	jnz	no_hd0		! error code != 0 -> bad
	or	dl,dl
	jz	no_hd0		! dl contains the number of harddisks
	push	dx
	mov	dl,dh
	xor	dh,dh
	inc	dx		! no. of heads in DX
	mov	bx,cx
	and	bx,#0x3f	! bx -> no. of sectors
	mov	al,ch
	mov	ah,cl
	mov	cl, #6
	shr	ah,cl		! ax -> no. of cylinders
	mov	[0x80],dx
	mov	[0x82],bx
	mov	[0x84],ax
	pop	dx
	dec	dl		! only one harddisk?
	jz	no_hd1
no_hd0:	mov	ah,#0x10
	mov	dl,#0x81
	int	0x13
	jc	no_hd1
	or	ah,ah
	jnz	no_hd1
	mov	ah,#0x08	! check for second drive
	mov	dl,#0x81
	int	0x13
	jc	no_hd1
	or	ah,ah
	jnz	no_hd1
	mov	dl,dh
	xor	dh,dh
	inc	dx		! no. of heads in DX
	mov	bx,cx
	and	bx,#0x3f	! bx -> no. of sectors
	mov	al,ch
	mov	ah,cl
	mov	cl,#6
	shr	ah,cl		! ax -> no. of cylinders
	mov	[0x86],dx
	mov	[0x88],bx
	mov	[0x8a],ax
no_hd1:	ret
#endif


!
! The processor name must not be longer than 15 characters!
!
#if !defined(CONFIG_ROMCODE) || defined(CONFIG_CPU_8086)
p8088:	.ascii "8088"
	db 0
p8086:	.ascii "8086"
	db 0
pv20:	.ascii "NEC V20"
	db 0
pv30:	.ascii "NEC V30"
	db 0
p80188:	.ascii "80188"
	db 0
p80186:	.ascii "80186"
	db 0
#endif
#if !defined(CONFIG_ROMCODE) || defined(CONFIG_CPU_80286)
p80286:	.ascii "80286"
	db 0
#endif
#if !defined(CONFIG_ROMCODE) || defined(CONFIG_CPU_80386)
p80386:	.ascii "80386"
	db 0
#endif
#if !defined(CONFIG_ROMCODE) || defined(CONFIG_CPU_80486)
p80486:	.ascii "80486 no CPUID"
	db 0
pvm86:	.ascii "VM86"
	db 0
#endif
!
! Here is the CPU id stored
!
v_id:	db 0,0,0,0
v_id2:	db 0,0,0,0
v_id3:	db 0,0,0,0
	db 0
!
! FPU names. must be not longer than 15 characters!
!
f_none:	.ascii "no fpu"
	db 0
#ifdef CONFIG_HW_FPU
f8087:	.ascii "8087"
	db 0
f80287:	.ascii "80287"
	db 0
f80387: .ascii "80387 or above"
	db 0
#endif
fpu_t:	.word f_none
#ifdef CONFIG_HW_FPU
         .word f8087, f80287, f80387
#endif

!
! Known 486 CPUs
!
#if !defined(CONFIG_ROMCODE) || defined(CONFIG_CPU_80486)
m486_0:	.ascii "486 model 0"
	db 0
m486_1:	.ascii "486DX"
	db 0
m486_2:	.ascii "486SX"
	db 0
m486_3:	.ascii "486DX/2"
	db 0
m486_4:	.ascii "486 model 4"
	db 0
m486_5:	.ascii "486SX/2"
	db 0
m486_6:	.ascii "486 model 6"
	db 0
m486_7:	.ascii "486 DX/2-WB"
	db 0
m486_8:	.ascii "486 DX/4"
	db 0
m486_9:	.ascii "486 DX/4-WB"
	db 0
m486_10: .ascii "486 model 10"
	db 0
m486_11: .ascii "486 model 11"
	db 0
m486_12: .ascii "486 model 12"
	db 0
m486_13: .ascii "486 model 13"
	db 0
m486_14: .ascii "Am5x86-WT"
	db 0
m486_15: .ascii "Am5x86-WB"
	db 0

m486_t:	.word m486_0,  m486_1,  m486_2,  m486_3
	.word m486_4,  m486_5,  m486_6,  m486_7
	.word m486_8,  m486_9,  m486_10, m486_11
	.word m486_12, m486_13, m486_14, m486_15
!
! Known Pentium CPUs
!
m586_0:	.ascii "586 model 0"
	db 0
m586_1:	.ascii "Pentium 60/66"
	db 0
m586_2: .ascii "Pentium 75+"
	db 0
m586_3: .ascii "Ovrdrv PODP5V83"
	db 0
m586_x:	.ascii "unknown 586"
	db 0

m586_t: .word m586_0, m586_1, m586_2, m586_3, m586_x

!
! Known Pentium Pro CPUs
!
m686_0: .ascii "PPro A-step"
	db 0
m686_1:	.ascii "Pentium Pro"
	db 0
m686_x:	.ascii "unknown 686"
	db 0
m686_t: .word m686_0, m686_1, m686_x

m_x86:	.ascii "Unknown Proc."
	db 0
#endif

#ifdef CONFIG_HW_FPU
copro:	dw 0
#endif

no_sig_mess:	.ascii	"No ELKS setup signature found ..."
		db	0x00


! This must be last
setup_sig1:	.word	SIG1
setup_sig2:	.word	SIG2

.text
endtext:
.data
enddata:
.bss
endbss:
