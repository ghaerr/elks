;	Assembler bootstrap hooks. This is called by setup

	.text
	.globl _main

; note: the next 3 int 3 instructions are part of the kernel_restart fix
; for protected mode.
	int 3
	int 3
	int 3

;	Setup passes these in registers (it used to use the stack, but that
;	would clobber the ELKS kernel)

_main:
        mov __endtext, bx
        mov __enddata, si
        add si, dx
        mov __endbss, si
	mov	ax,ds
	pushf
	pop bx
	cli
	mov	ss,ax
	mov	sp,#_bootstack
	push bx
	popf

	.extern _pmodekern_init
	call _relocate_kernel
	call _arch_boot
	call _pmodekern_init
	
; Break point if it returns

	int	3

_arch_boot:
	mov di,__enddata
	mov cx,__endbss

	sub cx,di
	xor ax,ax
	shr cx,#1
	rep
	stosw
	ret

; this routine moves the ELKS kernel to where it won't be clobbered by our bss
;   (which doesn't exist yet, but when it does it will extend into the area
;   that the kernel is sitting in now)
;
; (actually, if arch_cpu is less than 6, it moves the ELKS kernel over our
;   kernel and initializes it. no sense in keeping pmode support code around
;   on a system that won't handle it.)

_relocate_kernel:

; Aacckk! when someone moved the setup variables (by DEF_INITSEG) to 0x1000
;   linear, they didn't fix the comment at the beginning of setup.S!
;   this took me 3 tries to figure out. after figuring it out I remembered:
;   "Ignore the comments, they can be terribly misleading. Debug only code."
; If you change DEF_INITSEG, change the next line too.
	mov ax, #0x0100
	mov es, ax
	seg es
	mov al, [0x20]
	mov _arch_cpu, al
	cmp al, #6

; I really hate this assembler. it doesn't have the sense to code contitional
; branch beyond 127 bytes. plus, it doesn't respond to the jmp opcode the
; same way as every other 80x86 assembler in existance.

	jnb _relocate_kernel_0
	br _delete_self

_relocate_kernel_0:
	
	mov ax, __enddata
	add ax, #0x0f
	mov cl, #0x04
	shr ax, cl
	mov dx, ds
	add dx, ax
	mov es, dx

; right now, es:0 should point to the 32-byte minix header for the ELKS kernel.
; we need to determine it's size and move it to just beyond the end of our bss.
; while we're at it we may as well store a pointer to it's new location where
; we can find it when it comes time to actually start the kernel proper.

	seg es
	cmp [0], #0x0301
	jz _relocate_kernel_1
	br _bogus_magic

_relocate_kernel_1:

; the value 0x20 here is for separate I/D. if the kernel acquires different
; characteristics this value may have to change, as may the rest of this code

	seg es
	cmp byte [2], #0x20
	jz _relocate_kernel_2
	br _bogus_magic

_relocate_kernel_2:

; if anyone cares to tighten this up, be my guest.

; calculate the number of paragraphs to move by and store it into __elksheader

	mov ax, __endbss
	sub ax, __enddata
	add ax, #0xf
	shr ax, cl
	mov __elksheader, ax

; first, move the data segment

	push dx
	push ds
	push es
	seg es
	mov ax, [8]
	add ax, #0xf
	mov cl, #4
	shr ax, cl
	add dx, ax
	mov ax, __elksheader
	mov ds, dx
	add dx, ax
	seg es
	mov cx, [0xc]
	mov es, dx
	mov bx, cx
	shr cx, #1
	dec bx
	dec bx
	mov si, bx
	mov di, bx
	std
	repz
	movsw
	pop es
	pop ds
	pop dx

; then, the code segment

	push dx
	push ds
	push es
	add dx, #2
	mov ax, __elksheader
	mov cl, #4
	mov ds, dx
	add dx, ax
	seg es
	mov cx, [8]
	mov es, dx
	mov bx, cx
	shr cx, #1
	dec bx
	dec bx
	mov si, bx
	mov di, bx
	repz
	movsw
	pop es
	pop ds
	pop dx

; and last, the header

	push dx
	push ds
	push es
	mov ax, es
	mov bx, ax
	add ax, __elksheader
	mov __elksheader, ax
	mov es, ax
	mov ds, bx
	mov cx, #0x10
	mov si, #0x1e
	mov di, #0x1e
	repz
	movsw
	pop es
	pop ds
	pop dx

	push ds
	pop es

; whoops. forgot to clear the direction flag. _arch_init carefully clobbered
; my stack. the result was a return to the beginning. this took three days and
; a debugger constructed expressly for the purpose to fix. how annoying. :-/

	cld
	ret

; what is the correct response when we can't find the kernel image?

_bogus_magic:
	dw 0xfeeb
	
_delete_self:

; no sense in hanging around here...

; (does anyone want to finish this routine? it has to relocate a small chunk
;   of code to somewhere else and use said code to relocate the kernel to
;   DEF_INITSEG and transfer to it as if it were called directly from setup)

	db 0xeb, 0xfe

;	Segment beginnings	

	.data
	.globl __endtext
	.globl __enddata
	.globl __endbss
	.globl __elksheader
	.globl _arch_cpu
	.zerow	384
	.globl _bootstack

_bootstack:

; 768 byte boot stack. Print sp in wake_up and you'll see that more than
; 512 bytes of stack are used! Must be in data as its in use when we wipe
; the BSS

__endtext:
	.word	0

__enddata:
	.word	0

__endbss:
	.word	0

__elksheader:
	.word   0

_arch_cpu:
	.byte   0
	.bss

__sbss:
