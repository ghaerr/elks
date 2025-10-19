;*****************************************************************************
;*
;*                            Open Watcom Project
;*
;* Copyright (c) 2002-2019 The Open Watcom Contributors. All Rights Reserved.
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
; __FDI8 converts double into signed 64-bit integer
; __FDU8 converts double into unsigned 64-bit integer
;
; Input:    AX BX CX DX = double
; Output:   AX BX CX DX = 64-bit signed/unsigned integer
; Volatile: None
;
; 16-mar-2000     SJHowe        initial implementation
;

include mdef.inc
include struct.inc

        modstart        fdi8086

        xdefp   __FDI8
        xdefp   __FDU8

        defpe   __FDI8
        push    SI                      ; save register
        mov     SI,7FE0h + (63 * 32)    ; maximum exponent 2^63 allowed
        call    __FD8                   ; convert float to signed __int64
        pop     SI                      ; restore register
        ret                             ; return
        endproc __FDI8

        defpe   __FDU8
        push    SI                      ; save register
        mov     SI,7FE0h + (64 * 32)    ; maximum exponent 2^64 allowed
        call    __FD8                   ; convert float to unsigned __int64
        pop     SI                      ; restore register
        ret                             ; return
        endproc __FDU8

__FD8   proc    near
        or      ax,ax           ; check sign bit
        jns     __FDAbs         ; treat as unsigned if positive
        call    __FDAbs         ; otherwise convert number

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
        ret                     ; and return
        endproc __FD8

__FDAbs proc near
        rol     ax,1            ; move sign bit out of way
        cmp     ax,7FE0h        ; less than +/-1.0?
        jb      ret_zero        ; yes, return 0
        cmp     ax,si           ; more than max exponent?
        jae     ret_max         ; yes, return max
        shr     ax,1            ; eliminate sign bit

        mov     si,ax           ; save exponent
        and     ax,0Fh          ; just mantissa
        or      ax,10h          ; set implied 1

        shr     si,1            ; move exponent
        shr     si,1            ; :
        shr     si,1            ; :
        shr     si,1            ; :
        inc     si              ; adjust exponent

; need to shift AL left or right depending on exponent
; note: we cannot lose bits from DX on shifting right, all bits MUST be
; preserved. Therefore use AH as it is spare (the only register spare).

        test    si,4            ; shift left or right (or none)?
        jne     shift_left_start; jump if need to shift left

shift_right:
        shr     al,1            ; shift mantissa right 1 bit
        rcr     bx,1            ; :
        rcr     cx,1            ; :
        rcr     dx,1            ; :
        rcr     ah,1            ; :
        inc     si              ; increment exponent
        test    si,4            ; this bit set ?
        je      shift_right     ; no, loop again
        jmp     prepare_table_jmp

shift_left:
        _shl    dx,1
        _rcl    cx,1
        _rcl    bx,1
        _rcl    ax,1
shift_left_start:
        dec     si              ; decrement exponent
        test    si,4            ; shift left again ?
        jne     shift_left

; at this point
; al;bx;cx;dx;ah = mantissa, significant byte is in correct postion within al
; si = badly mangled exponent of which only bits 3-5 are now relevant

prepare_table_jmp:
        and     si,38h          ; eliminate surplus bits in exponent
        shr     si,1            ; prepare for a jump
        shr     si,1            ; :
        jmp     word ptr cs:sigbyte_table[si] ; jump to table

ret_zero:
        xor     ax,ax           ; return 0
        xor     bx,bx           ; :
        xor     cx,cx           ; :
        xor     dx,dx           ; :
        ret                     ; :

ret_max:
        mov     ax,0FFFFh       ; return maximum
        add     si,783Fh        ; complement of 7FE0h + (63 * 32)
                                ; carry set if unsigned, clear if integer
        mov     bx,ax           ; :
        mov     cx,ax           ; :
        mov     dx,ax           ; :
        rcr     ax,1            ; 0x7FFF if signed, 0xFFFF if unsigned
        ret                     ; return

sigbyte_table:
        DW      byte0_significant
        DW      byte1_significant
        DW      byte2_significant
        DW      byte3_significant
        DW      byte4_significant
        DW      byte5_significant
        DW      byte6_significant
        DW      byte7_significant

;AL;BX;CX;DX;AH = mantissa

byte0_significant:
        mov     dl,al           ; move sig byte 0
        xor     dh,dh           ; zero byte 1
        xor     cx,cx           ; zero bytes 2-3
        xor     bx,bx           ; zero bytes 4-5
        xor     ax,ax           ; zero bytes 6-7
        ret                     ; return

byte1_significant:
        mov     dh,al           ; move sig byte 1
        mov     dl,bh           ; move sig byte 0
        xor     cx,cx           ; zero bytes 2-3
        xor     bx,bx           ; zero bytes 4-5
        xor     ax,ax           ; zero bytes 6-7
        ret                     ; return

byte2_significant:
        mov     cl,al           ; move sig byte 2
        mov     dx,bx           ; move sig bytes 1-0
        xor     ch,ch           ; zero byte 3
        xor     ax,ax           ; zero bytes 6-7
        xor     bx,bx           ; zero bytes 4-5
        ret                     ; return

byte3_significant:
        mov     dl,ch           ; move sig byte 0
        mov     dh,bl           ; move sig byte 1
        mov     cl,bh           ; move sig byte 2
        mov     ch,al           ; move sig byte 3
        xor     bx,bx           ; zero bytes 4-5
        xor     ax,ax           ; zero bytes 6-7
        ret                     ; return

byte4_significant:
        mov     dx,cx           ; move sig bytes 1-0
        mov     cx,bx           ; move sig bytes 3-2
        mov     bl,al           ; move sig byte 4
        xor     bh,bh           ; zero byte 5
        xor     ax,ax           ; zero bytes 6-7
        ret                     ; return

byte5_significant:
        mov     dl,dh           ; move sig byte 0
        mov     dh,cl           ; move sig byte 1
        mov     cl,ch           ; move sig byte 2
        mov     ch,bl           ; move sig byte 3
        mov     bl,bh           ; move sig byte 4
        mov     bh,al           ; move sig byte 5
        xor     ax,ax           ; zero bytes 6-7
        ret                     ; return

byte6_significant:
        xor     ah,ah           ; zero byte 7
        ret                     ; return

byte7_significant:
        xchg    ah,al           ; shuffle sig byte 7 down to dl
        xchg    al,bh           ; :
        xchg    bh,bl           ; :
        xchg    bl,ch           ; :
        xchg    ch,cl           ; :
        xchg    cl,dh           ; :
        xchg    dh,dl           ; :
        ret                     ; return

        endproc __FDAbs

        endmod
        end

