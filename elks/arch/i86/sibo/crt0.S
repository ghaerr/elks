;	Assembler bootstrap hooks. This is called by setup

	.text
	.globl _main
	.extern	_start_kernel
	.extern _arch_boot

_main:
	cli
	mov al, #0x00
	out 0x15, al
	out 0x08, al

;	Copy the code segment to 0x04F0:0

	push ds
	push es

	mov ax, cs
	mov ds, ax
	mov ax, #0x04F0
	mov es, ax

	mov cx, #0x8000
	mov di, #0x0000
	mov si, #0x0000

codecopy:
	movsw
	loop codecopy

	mov ax, #0x04F0
	mov cs, ax

;	now running from new location!!!

	pop es
	pop ds

;	copy the Data segment to 0x14F0:0 (assumes full 0xffff for code)

	mov ax, #0x14F0
	mov es, ax

	mov cx, #0x8000
	mov di, #0x0000
	mov si, #0x0000
	cld

datacopy:
	movsw
	loop datacopy

	mov ax, #0x14F0
	mov ds, ax
	mov ss, ax

;	Set the page registers for fixed low memory

	mov al, #0x06
	out 0x28, al
	mov al, #0x07
	out 0x29, al

;	try moving the stack to the end of the data segment (as that's where it is expected to be

	mov ax, #0xFFFF
	mov sp, ax

	call	_arch_boot
	sti

	call	_start_kernel

; Break point if it returns
;	int	3
;
;	Segment beginnings

	.data
	.globl __endtext
	.globl __enddata
	.globl __endbss
	.zerow	384

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
	.bss

__sbss:
