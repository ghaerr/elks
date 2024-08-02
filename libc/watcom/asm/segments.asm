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
;* Description:  Watcom C segment ordering and null pointer and init/fini libc
;*
;*****************************************************************************


include mdef.inc

        name    segments
        assume  nothing

DGROUP group _NULL,_AFTERNULL,CONST,STRINGS,_DATA,DATA,XIB,XI,XIE,YIB,YI,YIE,_BSS,STACK

if (_MODEL and _BIG_CODE) eq 0

; BEGTEXT section used in near code (small and compact) models only
; this guarantees that no function pointer will equal NULL
; (WLINK will keep segment 'BEGTEXT' in front)
; This segment must be at least 2 bytes in size to avoid confusing the
; signal function.
; need a symbol defined here to prevent the dead code elimination from
; eliminating the segment.
; (the int 3h is useful for quickly revealing jumps to NULL code pointers)

BEGTEXT  segment word public 'CODE'
        assume  cs:BEGTEXT
        int     3
        int     3
_start  label   byte
        public  _start
        xrefp   _start_crt0
        dojmp   _start_crt0
        assume  cs:nothing
BEGTEXT  ends

endif

_TEXT   segment word public 'CODE'

FAR_DATA segment byte public 'FAR_DATA'
FAR_DATA ends

        assume  ds:DGROUP

_NULL   segment para public 'BEGDATA'
;       public  __nullarea
;__nullarea label word
        db      'nul'
        db      0
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

XIB     segment word public 'DATA'
_Start_XI label byte
        public  "C",_Start_XI
XIB     ends

XI      segment word public 'DATA'
XI      ends

XIE     segment word public 'DATA'
_End_XI label byte
        public  "C",_End_XI
XIE     ends

YIB     segment word public 'DATA'
_Start_YI label byte
        public  "C",_Start_YI
YIB     ends

YI      segment word public 'DATA'
YI      ends

YIE     segment word public 'DATA'
_End_YI label byte
        public  "C",_End_YI
YIE     ends

_BSS    segment word public 'BSS'
        ;extrn   _edata                  : byte  ; end of DATA (start of BSS)
        ;extrn   _end                    : byte  ; end of BSS  (start of HEAP/STACK)
_BSS    ends

;STACK_SIZE      equ     1000h
STACK   segment para stack 'STACK'
        ;db      (STACK_SIZE) dup(?)
STACK   ends

        assume  nothing

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

        end
