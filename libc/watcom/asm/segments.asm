;*****************************************************************************
;*
;*                            Open Watcom Project
;*
;* Copyright (c) 2002-2017 The Open Watcom Contributors. All Rights Reserved.
;*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
;*
;*  ========================================================================
;*
;*    This file contains Original Code and/or Modifications of Original
;*    Code as defined in and that are subject to the Sybase Open Watcom
;*    Public License version 1.0 (the 'License'). You may not use this file
;*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
;*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
;*    provided with the Original Code and Modifications, and is also
;*    available at www.sybase.com/developer/opensource.
;*
;*    The Original Code and all software distributed under the License are
;*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
;*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
;*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
;*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
;*    NON-INFRINGEMENT. Please see the License for the specific language
;*    governing rights and limitations under the License.
;*
;*  ========================================================================
;*
;* Description:  Watcom C segment ordering and null pointer section offsets
;*
;*****************************************************************************


        name    cstart
        assume  nothing

 ;DGROUP group _NULL,_AFTERNULL,CONST,STRINGS,_DATA,DATA,XIB,XI,XIE,YIB,YI,YIE,_BSS,STACK
 DGROUP group _NULL,_AFTERNULL,CONST,STRINGS,_DATA,DATA,_BSS

; this guarantees that no function pointer will equal NULL
; (WLINK will keep segment 'BEGTEXT' in front)
; This segment must be at least 2 bytes in size to avoid confusing the
; signal function.
; need a symbol defined here to prevent the dead code elimination from
; eliminating the segment.
; (the int 3h is useful for quickly revealing jumps to NULL code pointers)

BEGTEXT  segment word public 'CODE'
        assume  cs:BEGTEXT
        int     3h
        nop
        public __begtext
__begtext label byte
        assume  cs:nothing
BEGTEXT  ends

_TEXT   segment word public 'CODE'

FAR_DATA segment byte public 'FAR_DATA'
FAR_DATA ends

        assume  ds:DGROUP

_NULL   segment para public 'BEGDATA'
__nullarea label word
        db      'nul'
        db      0
        public  __nullarea
_NULL   ends

_AFTERNULL segment word public 'BEGDATA'
        ;dw      0                       ; nullchar for string at address 0
_AFTERNULL ends

CONST   segment word public 'DATA'
CONST   ends

STRINGS segment word public 'DATA'
STRINGS ends

_DATA   segment word public 'DATA'
_DATA   ends

DATA    segment word public 'DATA'
DATA    ends

_BSS    segment word public 'BSS'
        ;extrn   _edata                  : byte  ; end of DATA (start of BSS)
        ;extrn   _end                    : byte  ; end of BSS (start of STACK)
_BSS    ends

;STACK_SIZE      equ     1000h
;STACK   segment para stack 'STACK'
        ;db      (STACK_SIZE) dup(?)
;STACK   ends

        assume  nothing
        public  _cstart_

        assume  cs:_TEXT

_cstart_ proc near
        dw      __begtext               ; make sure dead code elimination
                                        ; doesn't kill BEGTEXT segment
_cstart_ endp

if 0
        INIT_VAL        equ 0101h
        NUM_VAL         equ 16
NullAssign      db      '*** NULL assignment detected',0
;       don't touch AL in __exit, it has the return code
__exit  proc
        public  "C",__exit
        push    ax                      ; save return code on stack
        mov     dx,DGROUP
        mov     ds,dx
        cld                             ; check lower region for altered values
        lea     di,__nullarea           ; set es:di for scan
        mov     es,dx
        mov     cx,NUM_VAL
        mov     ax,INIT_VAL
        repe    scasw
        je      L7
;
; low memory has been altered
;
        pop     bx                      ; restore return code
        mov     ax,offset NullAssign    ; point to msg
        mov     dx,cs                   ; . . .

; input: ( char __far *msg, int rc ) always in registers
;       DX:AX - far pointer to message to print
;       BX    - exit code

__exit  endp
endif

_TEXT   ends

        end     _cstart_
