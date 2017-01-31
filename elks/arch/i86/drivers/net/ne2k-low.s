;------------------------------------------------------------------------------
; NE2K driver - low part - test routines
;------------------------------------------------------------------------------

	IMPORT _main

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

	EXPORT _idle

	ENTRY _entry

	END

;------------------------------------------------------------------------------
