;*****************************************************************************
;*
;*                            Open Watcom Project
;*
;* Copyright (c) 2002-2022 The Open Watcom Contributors. All Rights Reserved.
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
;* Description:  Floating-point modulo and string to double conversion (8086).
;*
;*****************************************************************************


;       double modf( double value, double *iptr );
;
; Description:
;   The modf function breaks the argument value into integral and
;   fractional parts, each of which has the same sign as the argument.
;   It stores the integral part as a double in the object pointed to
;   by iptr.
;
; Returns:
;   The modf function returns the signed fractional part of value.
;
;  18-oct-86    ...             Fraction in Modf not computed right
;                               significant digits
;
include mdef.inc
include struct.inc

        modstart        amodf086

        xdefp   __ModF
        xdefp   __ZBuf2F

;[][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
;[]
;[] __ModF
;[]
;[]     void __ModF( double near *AX, double near *DX );
;[]     Input:  SS:AX       - pointer to double precision float
;[]             SS:DX       - place to store integral part
;[]     Output: SS:[AX]     - fractional part of value.
;[]             SS:[DX]     - integral part of value
;[]
;[][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

Masks   db      00h,080h,0C0h,0E0h,0F0h,0F8h,0FCh,0FEh

        defpe   __ModF
        push    BP              ; save BP
        push    SI              ; save SI
        push    DI              ; save DI
        push    CX              ; save CX
        push    BX              ; save BX
        mov     SI,DX           ; get address for ipart
        mov     BP,AX           ; get address of float
        mov     AX,6[BP]        ; get float
        mov     BX,4[BP]        ; . . .
        mov     CX,2[BP]        ; . . .
        mov     DX,0[BP]        ; . . .
        xchg    SI,BP           ; flip pointers
        mov     6[BP],AX        ; store integral part of value
        mov     4[BP],BX        ; . . .
        mov     2[BP],CX        ; . . .
        mov     [BP],DX         ; . . .
        _guess                  ; guess: fraction is zero
          mov   DI,AX           ; - get exponent
          and   DI,7FF0h        ; - get exponent part of R into DI
          je    done            ; - set integer part to 0 if exponent = 0
          cmp   DI,3FF0h+(52*16); - check for exponent > 52
          _quif b               ; - quit if fraction not zero
          xchg  BP,SI           ; - get address of fractional part
done:     sub   AX,AX           ; - set fraction(or integer) to 0
          mov   6[BP],AX        ; - . . .
          mov   4[BP],AX        ; - . . .
          mov   2[BP],AX        ; - . . .
          mov   0[BP],AX        ; - . . .
          pop   BX              ; - restore BX
          pop   CX              ; - restore CX
          pop   DI              ; - restore DI
          pop   SI              ; - restore SI
          pop   BP              ; - restore BP
          ret                   ; - return
        _endguess               ; endguess
        cmp     DI,3FF0h        ; quit if number >= 1.0
        jb      done            ; quit if number >= 1.0
          push  AX              ; - save sign
          push  DI              ; - save exponent
          sub   DI,3FF0h-4*16   ; - remove bias and add 4
          mov   AX,DI           ; - ...
          _shl  AX,1            ; - times 2
          mov   BL,AL           ; - get low order 3 bits of exponent
          rol   BL,1            ; - rotate into position
          rol   BL,1            ; - ...
          rol   BL,1            ; - ...
          xor   BH,BH           ; - zero high byte for indexing
          mov   AL,AH           ; - get high part of exponent
          mov   AH,BH           ; - zero high part
          mov   DI,AX           ; - ...
          inc   DI              ; - add 1
          mov   AH,Masks[BX]    ; - get mask for last 3 bits of exponent
          sub   BX,BX           ; - initialize mask to 0
          sub   CX,CX           ; - ...
          sub   DX,DX           ; - ...
          _guess                ; - guess: set up mask in AX:BX:CX:DX
           mov    AL,AH         ; - - assume last part
           dec    DI            ; - - decrement exponent
           _quif  e             ; - - quit if done
           mov    AL,0FFh       ; - - set mask to FF
           mov    BH,AH         ; - - get last part of mask
           dec    DI            ; - - decrement exponent
           _quif  e             ; - - quit if done
           mov    BH,0FFh       ; - - set mask to FF
           mov    BL,AH         ; - - get last part of mask
           dec    DI            ; - - decrement exponent
           _quif  e             ; - - quit if done
           mov    BL,0FFh       ; - - set mask to FF
           mov    CH,AH         ; - - get last part of mask
           dec    DI            ; - - decrement exponent
           _quif  e             ; - - quit if done
           mov    CH,0FFh       ; - - set mask to FF
           mov    CL,AH         ; - - get last part of mask
           dec    DI            ; - - decrement exponent
           _quif  e             ; - - quit if done
           mov    CL,0FFh       ; - - set mask to FF
           mov    DH,AH         ; - - get last part of mask
           dec    DI            ; - - decrement exponent
           _quif  e             ; - - quit if done
           mov    DH,0FFh       ; - - set mask to FF
           mov    DL,AH         ; - - get last part of mask
          _endguess             ; - endguess
          mov   AH,0FFh         ; - set high part of mask
          and   6[BP],AX        ; - mask off the remaining fraction bits
          and   4[BP],BX        ; - . . .
          and   2[BP],CX        ; - . . .
          and   0[BP],DX        ; - . . .
          not   AX              ; - complement the mask to get fractional part
          not   BX              ; - . . .
          not   CX              ; - . . .
          not   DX              ; - . . .
          mov   BP,SI           ; - get address of fractional part
          and   AX,6[BP]        ; - get fraction bits
          and   BX,4[BP]        ; - . . .
          and   CX,2[BP]        ; - . . .
          and   DX,0[BP]        ; - . . .
          pop   DI              ; - restore exponent
          call  Norm            ; - normalize the fraction
          pop   SI              ; - restore sign
          or    AX,AX           ; - if fraction is not 0
          _if   ne              ; - then
            and   SI,8000h      ; - - isolate sign
            or    AX,SI         ; - - set sign in fractional part
          _endif                ; - endif
          mov   6[BP],AX        ; - store fractional part
          mov   4[BP],BX        ; - . . .
          mov   2[BP],CX        ; - . . .
          mov   0[BP],DX        ; - . . .
        pop     BX              ; restore BX
        pop     CX              ; restore CX
        pop     DI              ; restore DI
        pop     SI              ; restore SI
        pop     BP              ; restore BP
        ret                     ; return
        endproc __ModF


;<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
;<>                                                                   <>
;<>   __ZBuf2F - convert buffer of significant digits into floating   <>
;<>             void __ZBuf2F( char near *buf, double near *value )   <>
;<>                                                                   <>
;<>   input:  AX - address of buffer of significant digits            <>
;<>           DX - place to store value                               <>
;<>   output: [DX]        - floating point number                     <>
;<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>

        defpe   __ZBuf2F
        push    BP              ; save BP
        push    SI              ; save SI
        push    DI              ; save DI
        push    CX              ; save CX
        push    BX              ; save BX
        push    DX              ; save pointer to result
        mov     SI,AX           ; get address of buffer
        sub     DX,DX           ; set 54-bit integer to 0
        mov     DI,DX           ; . . .
        mov     CX,DX           ; . . .
        mov     BX,DX           ; . . .
        _loop                   ; loop (convert digits into 54-bit int)
          mov   AL,ss:[SI]      ; - get next digit
          test  AL,AL           ; - quit if at end of buffer
          _quif e               ; - . . .

;[]  multiply current value in DL:BX:CX:DI by 10

          push  DX              ; - save current value
          push  BX              ; - . . .
          mov   AX,CX           ; - . . .
          mov   BP,DI           ; - . . .

          _shl  DI,1            ; - multiply number by 4
          _rcl  CX,1            ; -   by shifting left 2 places
          _rcl  BX,1            ; - . . .
          _rcl  DX,1            ; - . . .
          _shl  DI,1            ; - . . .
          _rcl  CX,1            ; - . . .
          _rcl  BX,1            ; - . . .
          _rcl  DX,1            ; - . . .
          add   DI,BP           ; - add original value
          adc   CX,AX           ; -  (this will make it times 5)
          pop   AX              ; - . . .
          adc   BX,AX
          pop   AX
          adc   DX,AX
          _shl  DI,1            ; - shift left to make it times 10
          _rcl  CX,1            ; - . . .
          _rcl  BX,1            ; - . . .
          _rcl  DX,1            ; - . . .
          mov   AL,ss:[SI]      ; - add in current digit
          and   AX,000Fh        ; - . . .
          add   DI,AX           ; - . . .
          adc   CX,0            ; - . . .
          adc   BX,0            ; - . . .
          adc   DX,0            ; - . . .
          inc   SI              ; - point to next digit in buffer
        _endloop                ; endloop
        mov     AX,DX           ; get high order word into AX
        mov     DX,DI           ; set up integer into AX BX CX DX

;[] Turn the integer in AX BX CX DX into a real number

        mov     DI,3FF0h+(52*16); set exponent
        call    Norm            ; convert the 52 bit integer to a float
        pop     BP              ; restore pointer to result
        mov     6[BP],AX        ; store result
        mov     4[BP],BX        ; . . .
        mov     2[BP],CX        ; . . .
        mov     0[BP],DX        ; . . .
        pop     BX              ; restore BX
        pop     CX              ; restore CX
        pop     DI              ; restore DI
        pop     SI              ; restore SI
        pop     BP              ; restore BP
        ret                     ; return to caller
        endproc __ZBuf2F


;[] Norm normalizes an unsigned real in AX BX CX DX, so that the implied '1'
;[] bit is in bit '4' of AX (it can be sent this way, if you wish) and
;[] expects the exponent to be in DI.  The real returned is in 'packed'
;[] format
;[]     SI is destroyed

Norm    proc    near            ; normalize floating point number
        sub     SI,SI           ; clear out SI
        or      SI,AX           ; see if the integer is zero
        or      SI,BX           ; . . .
        or      SI,CX           ; . . .
        or      SI,DX           ; . . .
        je      Z_52ret         ; if integer is zero, return to caller
        _loop                   ; loop
          or    AX,AX           ; -
          _quif ne              ; - quif AX <> 0
          mov   AL,BH           ; - shift integer left by 8 bits
          mov   BH,BL           ; - . . .
          mov   BL,CH           ; - . . .
          mov   CH,CL           ; - . . .
          mov   CL,DH           ; - . . .
          mov   DH,DL           ; - . . .
          xor   DL,DL           ; - . . .
          sub   DI,0080h        ; - exp <-- exp - 8
        _endloop                ; endloop
        test    AX,0FFF0h       ; see if we have to shift forward or backward
        _if     e               ; if (we haven't shifted msb into bit 53)
          _loop                 ; - loop
            sub DI,0010h        ; - - exp <-- exp - 1
            _shl DX,1           ; - - shift integer left by 1 bit
            _rcl CX,1           ; - - . . .
            _rcl BX,1           ; - - . . .
            _rcl AL,1           ; - - . . .
            test AL,0F0h        ;
          _until ne             ; - until( msb is shifted into bit 53 )
        _else                   ; else (we must shift to the right)
          test AX,0FFE0h        ; -
          je  done1             ; - if msb is bit 53, we are done
          _loop                 ; - loop
            add DI,0010h        ; - - exp <-- exp + 1
            shr AX,1            ; - - shift integer right by 1 bit
            rcr BX,1            ; - - . . .
            rcr CX,1            ; - - . . .
            rcr DX,1            ; - - . . .
            rcr SI,1            ; - - save lsb
            test AX,0FFE0h      ; -
          _until e              ; - until( msb is bit 53 )
          _rcl  SI,1            ; - get lsb
          adc   DX,0            ; - and use it to round off the number
          adc   CX,0            ; - . . .
          adc   BX,0            ; - . . .
          adc   AL,0            ; - . . .
        _endif                  ; endif
done1:  and     AX,000Fh        ; clear out implied bit
        or      AX,DI           ; put in exponent
Z_52ret:ret                     ; return
        endproc Norm

        endmod
        end
