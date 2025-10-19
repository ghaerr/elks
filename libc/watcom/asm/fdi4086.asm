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
include mdef.inc
include struct.inc

        modstart        fdi4086

        xdefp   __FDI4
        xdefp   __RDI4

        xdefp   __FDU4
        xdefp   __RDU4

;[][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
;[]
;[] __FDU4      convert double float AX;BX;CX;DX into 32-bit integer DX:AX
;[]     Input:  AX BX CX DX - double precision floating point number
;[]     Output: DX:AX       - 32-bit integer
;[]
;[][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
;       convert floating double to 4-byte integer with rounding

        defpe   __RDU4
        mov     DX,0080h+0020h  ; indicate we are rounding
        jmp     DtoI            ; do it

        defpe   __RDI4
        mov     DX,0080h+001Fh  ; indicate we are rounding
        jmp     DtoI            ; do it

        defpe   __FDI4
        mov     DX,001Fh        ; indicate we are truncating
        jmp     DtoI            ; do it

;       convert floating double to 4-byte integer with truncation

        defpe   __FDU4
        mov     DX,0020h        ; indicate we are truncating

DtoI:   _shl    AX,1            ; get sign
        rcr     DH,1            ; DH <-- sign
        shr     DH,1            ; shift sign bit over 1
        or      DH,DL           ; get rounding bit
        shr     AX,1            ; restore exponent to its place

;       high bit of DH is rounding bit
;       next bit is the sign bit

;<~> Shift real right four places so that exponent occupies an entire
;<~> word and the mantissa occupies the remaining words. We do not need
;<~> DX because we only need 32 sig digits

        shr     AX,1
        rcr     BX,1
        rcr     CX,1
        shr     AX,1
        rcr     BX,1
        rcr     CX,1
        shr     AX,1
        rcr     BX,1
        rcr     CX,1
        shr     AX,1
        rcr     BX,1
        rcr     CX,1
        sub     AX,03FEh        ; remove bias from exponent
        jl      DIzero          ; if |real| < .5 goto DIzero
        cmp     AX,20h          ; if exponent > 32
        jg      DIo_f           ; goto DIo_flow
        and     DL,3Fh          ; isolate # of significant bits
        cmp     AL,DL           ; quit if number too big
        jg      DIo_f           ; goto DIo_flow
        stc                     ; set carry and
        rcr     BX,1            ; restore implied 1/2 bit
        rcr     CX,1
        rcr     AH,1            ; save rounding bit
        mov     DL,32           ; # of bits to shift right = 32 - AL
        sub     DL,AL           ; ...
        mov     AL,DL           ; ...
        _guess                  ; guess: exponent >= 16
          cmp   AL,16           ; - quit if < 16
          _quif l               ; - ...
          mov   AH,CH           ; - save rounding bit
          mov   CX,BX           ; - shift right 16 bits
          sub   BX,BX           ; - zero high word
          sub   AL,16           ; - adjust exponent by 16
        _endguess               ; endguess
        _guess                  ; guess: exponent >= 8
          cmp   AL,8            ; - quit if < 8
          _quif l               ; - . . .
          mov   AH,CL           ; - save rounding bit
          mov   CL,CH           ; - shift right 8 bits
          mov   CH,BL           ; - . . .
          mov   BL,BH           ; - . . .
          xor   BH,BH           ; - zero high byte
          sub   AL,8            ; - adjust exponent by 8
        _endguess               ; endguess

        cmp     AL,0            ; if there are bits to shift
        _if     ne              ; then
          _loop                 ; - loop
            shr   BX,1          ; - - shift mantissa into integer
            rcr   CX,1          ; - - . . .
            rcr   AH,1          ; - - save rounding bit
            dec   AL            ; - - dec AL
            _quif e             ; - - quif AL = 0
            shr   BX,1          ; - - shift mantissa into integer
            rcr   CX,1          ; - - . . .
            rcr   AH,1          ; - - save rounding bit
            dec   AL            ; - - decrement exponent
          _until  e             ; - until done
        _endif                  ; endif
        and     AH,DH           ; trunc or rnd as determined by high bit of DH
        _shl    DH,1            ; get extra significant bit from mantissa
        adc     CX,0            ; add it to the integer to round it up
        adc     BX,0            ; . . .
        _shl    DH,1            ; get sign
        _if     c               ; if negative
          not   BX              ; - negate integer
          neg   CX              ; - . . .
          sbb   BX,-1           ; - . . .
        _endif                  ; endif
        mov     AX,CX           ; put result in correct registers
        mov     DX,BX           ; . . .
        ret                     ; return

DIo_f:
        mov     DX,8000h        ; set answer to largest negative number
        sub     AX,AX           ; . . .
        ret                     ; return
;       jmp     I4OverFlow      ; report overflow

DIzero: sub     AX,AX           ; set result to zero
        sub     DX,DX           ; . . .
        ret
        endproc __FDU4
        endproc __FDI4
        endproc __RDI4
        endproc __RDU4

        endmod
        end
