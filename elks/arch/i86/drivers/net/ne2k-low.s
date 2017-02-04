;------------------------------------------------------------------------------
; NE2K driver - low part - test routines
;------------------------------------------------------------------------------

	IMPORT  _main


; On Advantech SNMP-1000 SBC, the ethernet interrupt is INT0 (0Ch)

int_vect    EQU $0C


	.TEXT


;------------------------------------------------------------------------------
; Main entry point
;------------------------------------------------------------------------------

; Test program to be executed
; within single segment with CS=DS=ES=SS
; and stack already set on entry

_entry:

	push    ax
	push    ds
	push    es

	mov     ax, cs
	mov     ds, ax
	mov     es, ax

	call    _main

	pop     es
	pop     ds
	pop     ax

	iret


;------------------------------------------------------------------------------
; Idle routine
;------------------------------------------------------------------------------

; Halt the processor until next interrupt

_idle:

	hlt
	ret


;------------------------------------------------------------------------------
; Interrupt handler
;------------------------------------------------------------------------------

; Do nothing for test program

int_hand:

	push    ax
	push    dx

	; end of interrupt handling

	mov     ax, #int_vect
	mov     dx, #$FF22
	out     dx, ax

	pop     dx
	pop     ax

	iret


;------------------------------------------------------------------------------
; Interrupt setup
;------------------------------------------------------------------------------

_int_setup:

	push    ax
	push    bx
	push    ds

	xor     ax, ax
	mov     ds, ax

	mov     bx, #int_vect * 4

	mov     ax, #int_hand
	mov     [bx], ax
	mov     [bx + 2], cs

	pop     ds
	pop     bx
	pop     ax
	ret


;------------------------------------------------------------------------------
; Copy two strings
;------------------------------------------------------------------------------

; arg1 : string to
; arg2 : string from
; arg3 : copy length

_strncpy:

	push    bp
	mov     bp, sp

	push    ax
	push    cx
	push    si
	push    di

	mov     di, [bp + 4]
	mov     si, [bp + 6]
	mov     cx, [bp + 8]

	cld
	rep
	movsb

	pop     di
	pop     si
	pop     cx
	pop     ax
	pop     bp
	ret


;------------------------------------------------------------------------------
; Compare two strings
;------------------------------------------------------------------------------

; arg1 : string address 1
; arg2 : string address 2
; arg3 : compare length

; returns

; AX : 0 = equal, 1 = diff

_strncmp:

	push    bp
	mov     bp, sp

	push    cx
	push    si
	push    di

	mov     si, [bp + 4]
	mov     di, [bp + 6]
	mov     cx, [bp + 8]

	cld
	repz
	cmpsb

	jnz strcmp_diff

	xor     ax, ax
	jmp     strcmp_exit

strcmp_diff:

	mov     ax, #$FFFF

strcmp_exit:

	pop     di
	pop     si
	pop     cx
	pop     bp
	ret


;------------------------------------------------------------------------------

	EXPORT  _idle
	EXPORT  _int_setup

	EXPORT  _strncpy
	EXPORT  _strncmp

	ENTRY   _entry

	END

;------------------------------------------------------------------------------
