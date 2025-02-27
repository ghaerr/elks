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
; __FSI8 converts double into signed 64-bit integer
; __FSU8 converts double into unsigned 64-bit integer
;
; Input:    DX AX = float
; Output:   AX BX CX DX = 64-bit signed/unsigned integer
; Volatile: None
;
; 14-mar-2000     SJHowe        initial implementation

include mdef.inc
include struct.inc

        modstart        fsi8086

        xdefp   __FSI8
        xdefp   __FSU8

        defpe   __FSI8
        mov     cl,7fh+63       ; maximum number 2^63-1 + offset
        jmp     __FS8           ; convert to integer
        endproc __FSI8

        defpe   __FSU8
        mov     cl,7fh+64       ; maximum number 2^64-1 + offset

; jmp not needed, fall through
;       jmp     __FS8           ; convert to integer
        endproc __FSU8

__FS8   proc    near
        or      dx,dx           ; check sign bit
        jns     __FSAbs         ; treat as unsigned if positive
        call    __FSAbs         ; otherwise convert number

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
        endproc __FS8

;
__FSAbs proc near
        rol     dx,1            ; rotate sign bit out of way
        cmp     dh,7Fh          ; exponent less than that for 1.0F ?
        jb      ret_zero        ; yes, underflow => return 0
        cmp     dh,cl           ; exponent more than maximum ?
        jae     ret_max         ; yes, overflow => return max
        shr     dl,1            ; move mantissa
        or      dl,80h          ; set implied 1 of mantissa
        xor     bx,bx           ; need to zero bh, so do bl
        xor     cx,cx           ; need to zero ch, so do cl
        xchg    bl,dh           ; move exponent, dh is 0

; at this point
; DH = 0, DL;AX = mantissa
; CX = 0                ; in preparation for mantissa bits to be shifted in
; BH = 0, BL = exponent ; in preparation for jump table below

        add     bx,2            ; this is so, the bottom 3 bits of the exponent
                                        ; will be 0 when enough shifting has occured
        jmp     shift_start     ; start shift

shift_loop:
        shr     dl,1            ; shift mantissa right 1 bit
        rcr     ax,1            ; :
        rcr     ch,1            ; :
        inc     bx              ; increment exponent
shift_start:
        test    bx,7            ; bottom 3 bit clear?
        jne     shift_loop      ; no, loop again

        sub     bx,88H          ; correct exponent for jump table
        shr     bx,1            ; prepare to use jump table
        shr     bx,1            ; :

; at this point
; DH = 0, DL;AX;CH = mantissa, CL = 0
; BX = jump offset for jump table below

        jmp     word ptr cs:sigbyte_table[bx] ; jump to table

;
; underflow, 0, denormal in float => return 0
;
ret_zero:
        xor     ax,ax           ; return 0UI64
        xor     bx,bx           ; :
        xor     cx,cx           ; :
        xor     dx,dx           ; :
        ret                     ; :

;
; overflow, INF, NAN => return adjusted maximum
;
ret_max:
        mov     ax,0FFFFh       ; return maximum (but adjusted for signed/unsigned)
        not     cl              ; flip bits
        mov     bx,ax           ; :
        and     cl,1            ; now 0 or 1
        shr     ax,cl           ; now 7FFFh or FFFFh
        mov     dx,bx           ; :
        mov     cx,bx           ; :
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

comment $
oldbyte0_significant:

Jump label byte0_significant has been eliminated, because, since the first
3 instructions are effectively done, the next 3 instructions match the
trailing end of byte1_significant, therefore merge the two together.

;       xor     dh,dh           ; zero byte  1         ; already 0
;       mov     dl,dl           ; move sig byte 0      ; no move needed
;       xor     bx,bx           ; zero bytes 4-5       ; already 0
        xor     cx,cx           ; zero bytes 2-3
        xor     ax,ax           ; zero bytes 6-7
        ret                     ; return
$

byte1_significant:
        mov     dh,dl           ; move sig byte 1
        mov     dl,ah           ; move sig byte 0
        xor     bx,bx           ; zero bytes 4-5

byte0_significant:
        xor     cx,cx           ; zero bytes 2-3
        xor     ax,ax           ; zero bytes 6-7
        ret                     ; return

byte2_significant:
        mov     cl,dl           ; move sig byte 2
        xor     ch,ch           ; zero byte 3
        mov     dx,ax           ; move sig bytes 0-1
        xor     bx,bx           ; zero bytes 4-5
        xor     ax,ax           ; zero bytes 6-7
        ret                     ; return

byte3_significant:
        mov     cl,ah           ; move sig byte 2
        mov     dh,al           ; move sig byte 1
        xor     bx,bx           ; zero bytes 4-5
        xor     ax,ax           ; zero bytes 6-7
        xchg    ch,dl           ; move sig bytes 0,3
        ret                     ; return

byte4_significant:
        mov     bl,dl           ; move sig byte 4
        mov     dh,ch           ; move sig byte 1
        xor     dl,dl           ; sig byte 0 = 0 (no more mantissa bits)
        mov     cx,ax           ; move sig bytes 2-3
;       xor     bh,bh           ; zero byte 5           ; already 0
        xor     ax,ax           ; zero bytes 6-7
        ret                     ; return

byte5_significant:
        mov     bh,dl           ; move sig byte 5
        mov     cl,ch           ; move sig byte 2
        mov     bl,ah           ; move sig byte 4
        mov     ch,al           ; move sig byte 3
        xor     dx,dx           ; sig bytes 0-1 = 0 (no more mantissa bits)
        xor     ax,ax           ; zero bytes 6-7
        ret                     ; return

byte6_significant:
        mov     bx,ax           ; move sig bytes 4-5
        mov     al,dl           ; move sig byte 6
        xor     ah,ah           ; zero byte 7
;       mov     ch,ch           ; move sig byte 3       ; no move needed
;       xor     cl,cl           ; sig byte 2 = 0 (no more mantissa bits) ; already zero
        xor     dx,dx           ; sig bytes 0-1 = 0 (no more mantissa bits)
        ret                     ; return

byte7_significant:
        mov     bh,al           ; move sig byte 5
        mov     bl,ch           ; move sig byte 4
        mov     al,ah           ; move sig byte 6
        mov     ah,dl           ; move sig byte 7
        xor     cx,cx           ; sig bytes 3-2 = 0 (no more mantissa bits)
        xor     dx,dx           ; sig bytes 0-1 = 0 (no more mantissa bits)
        ret                     ; return
        endproc __FSAbs

        endmod
        end
