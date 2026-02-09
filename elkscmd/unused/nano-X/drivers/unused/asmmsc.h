; 30 Aug 92
; Copyright (c) 1999 Greg Haerr <greg@censoft.com>
; msc.h - asm.h include for MSC Compiler
;
; 8/30/92 changed small model to use _TEXT only for fixup overflows
; 7/26/92 v6.3 .center/.cexit macros for TSC
; 11/17/90 original version
;
; .header - start an assembly file
.header	macro
ifdef __SMALL__
_TEXT	SEGMENT  WORD PUBLIC 'CODE'
_TEXT	ENDS
else
ASM_TEXT	SEGMENT  WORD PUBLIC 'CODE'
ASM_TEXT	ENDS
endif
_DATA	SEGMENT  WORD PUBLIC 'DATA'
_DATA	ENDS
CONST	SEGMENT  WORD PUBLIC 'CONST'
CONST	ENDS
_BSS	SEGMENT  WORD PUBLIC 'BSS'
_BSS	ENDS
DGROUP	GROUP	CONST, _BSS, _DATA
ifdef __LARGE__
	ASSUME  CS: ASM_TEXT, DS: DGROUP, SS: DGROUP
endif
ifdef __MEDIUM__
	ASSUME  CS: ASM_TEXT, DS: DGROUP		; small data
endif
ifdef __SMALL__
	ASSUME  CS: _TEXT, DS: DGROUP			; small data
endif
_BSS	SEGMENT
_BSS	ENDS
	endm
;
; .cseg - start a code segment
.cseg	macro
ifdef __SMALL__
_TEXT      SEGMENT
	ASSUME	CS: _TEXT
else
ASM_TEXT      SEGMENT
	ASSUME	CS: ASM_TEXT
endif
	endm
;
; .cend - end a code segment
.cend	macro
ifdef __SMALL__
_TEXT	ENDS
else
ASM_TEXT	ENDS
endif
	endm
;
; .dseg - start a data segment
.dseg	macro
_DATA	segment word public 'DATA'
	endm
;
; .dsym - define data
.dsym	macro	name,type
	public	_&name
_&name	label	type
	endm
;
; .dend - end a data segment
.dend	macro
_DATA	ends
	endm
;
; .cextp name - declare an external procedure, use current model for near/far
.cextp	macro	name
if	LPROG
	extrn _&name:far
else
	extrn _&name:near
endif
name&@	equ	_&name
	endm
;
; .cextrn name,type - declare external C variable and type
.cextrn	macro	name,type
	extrn	_&name:type
name&@	equ	DGROUP:_&name
	endm
;
; .cproc name - used to start a C procedure
.cproc	macro	name
	public _&name
if	LPROG
	arg1	= 6
	_&name	proc	far
else
	arg1	= 4
	_&name	proc	near
endif
name&@	equ	_&name
	endm
;
; .cendp - end a C procedure
.cendp macro name
_&name	endp
	endm
;
; .center - enter C procedure
.center	macro
	push	bp
	mov	bp,sp
	endm
;
; .cexit - exit C procedure
.cexit	macro
	pop	bp
	ret
	endm

