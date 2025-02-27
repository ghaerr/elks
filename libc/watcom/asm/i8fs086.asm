;*****************************************************************************
;*
;*                            Open Watcom Project
;*
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
;* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
;*               DESCRIBE IT HERE!
;*
;*****************************************************************************


;
; __I8FD converts signed 64-bit integer into a float
; __U8FD converts unsigned 64-bit integer into a float
;
; Input:    AX BX CX DX = 64-bit signed/unsigned integer
; Output:   DX AX = float
; Volatile: None
;
; 12-jan-2000   SJHowe          initial implementation

include mdef.inc
include struct.inc

        modstart        i8fs086

        xrefp   __U4FS

        xdefp   __I8FS
        defp    __I8FS
        or      ax,ax           ; test top word for being 0, +ve or -ve
        je      chkuint48       ; 0 => check next word down within __U8FS
        jg      uint64          ; +ve => convert as unsigned __int64
                                ; -ve => need to negate before calling __U8FS

                                ; negate 64-bit integer

; rely on the crucial fact that the Intel instruction "not" does not alter
; any flags. Borrows from previous register needs to propagated in the
; direction of the most significant word.

        neg     dx              ; negate
        not     cx              ; :
        sbb     cx,0FFFFH       ; :
        not     bx              ; :
        sbb     bx,0FFFFH       ; :
        not     ax              ; :
        sbb     ax,0FFFFH       ; :
        lcall   __U8FS          ; convert as unsigned 64-bit
        or      dh,80h          ; set sign bit
        ret                     ; return
        endproc __I8FS

; Convert unsigned 64-bit integer to single precision float
; Input: [AX BX CX DX] = 64-bit integer
; Output: [DX AX] = 32-bit float

        xdefp   __U8FS
        defp    __U8FS
        or      ax,ax           ; check if 0
        jne     uint64          ; no, do 64-bit conversion
chkuint48:
        or      bx,bx           ; check if 0
        jne     uint48          ; no, do 48-bit conversion

; do a 32-bit unsigned conversion, use __U4FS
        mov     ax,dx           ; prepare registers for __U4FS
        mov     dx,cx           ; :
        jmp     __U4FS          ; call unsigned long => float cast function

; up to 48-bits set
uint48:
        mov     ax,bx           ; shift registers left 16 bits ...
        mov     bx,cx           ; :
        mov     ch,dh           ; :
        mov     dx,7F00h+(32*128); create float's exponent
        jmp     uint641         ; ... and continue as uint64 easy case

uint64:
        mov     dx,7F00h+(32*128); create float's exponent


; at this point
; dx holds exponent
; ax holds most significant bits (could be just 1)
; bx holds next significant bits (16)
; ch holds next significant bits (could be 8)

uint641:
        test    ah,ah           ; any bits set ?
        je      uint642         ; no, jump
        mov     ch,bl           ; :
        mov     bl,bh           ; :
        mov     bh,al           ; :
        mov     al,ah           ; :
        add     dh,40h          ; adjust exponent
        jmp     uint642

uint64loop:
        add     ch,ch           ; shift mantisa
        adc     bx,bx           ; :
        adc     al,al           ; :
        sub     dx,80h          ; correct exponent
uint642:
        test    al,80H          ; implied 1 seen ?
        jne     uint64loop      ; no loop again

        and     al,7Fh          ; get rid of implied 1...
        or      dl,al           ; ...and move mantissa into correct place

; use 25th bit of mantissa to round
        add     ch,ch           ; move 25th bit into carry
        adc     bx,0            ; round
        adc     dx,0            ; :
        mov     ax,bx           ; move mantissa lo word into correct place
        ret                     ; return
        endproc __U8FS

        endmod
        end
