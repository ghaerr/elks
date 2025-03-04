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


include mdef.inc
include struct.inc

        modstart        fprem086

        xdefp   __fprem_

;
;       void fprem( double x, double modulus, int *quot, double *rem )
;
        defpe   __fprem_
        push    BP              ; save BP
        mov     BP,SP           ; get access to parms
if _MODEL and _BIG_CODE
        add     BP,6+8          ; point to modulus
else
        add     BP,4+8          ; point to modulus
endif

        push    SI              ; save SI
        push    DI              ; save DI
        push    DX              ; save DX
        push    CX              ; save CX
        push    BX              ; save BX
        mov     DI,6[BP]        ; get most sig word of op2
        and     DI,7FF0h        ; isolate exponent of modulus
        _if     e               ; if modulus is 0       24-mar-89
          sub   AX,AX           ; - set result to 0
          sub   BX,BX           ; - ...
          sub   CX,CX           ; - ...
          sub   DX,DX           ; - ...
          jmp   _return         ; - return
        _endif                  ; endif
        mov     AX,-2[BP]       ; load x
        mov     BX,-4[BP]       ; ...
        mov     CX,-6[BP]       ; ...
        mov     DX,-8[BP]       ; ...
        push    AX              ; save sign of operand

        mov     SI,AX           ; get most sig word of op1
        mov     AH,6[BP]        ; get most sig byte of op2's mantissa
        and     SI,7FF0h        ; isolate exponent
        and     AX,0F0Fh        ; keep top 4 bits of mantissa
        or      AX,1010h        ; set implied one bit of mant.
        sub     SI,DI           ; calculate difference in exponents
        _if     ge              ; if operand >= modulus
          xor   DI,DI           ; - set quotient to 0
          _loop                 ; - loop
            _guess              ; - - guess
              cmp  AH,AL        ; - - - The purpose of this guess is to
              jb   try          ; - - - determine if the divisor will subtract
              ja   didnt_go     ; - - - from the dividend without a borrow, and to
              cmp  4[BP],BX     ; - - - branch to the appropriate routine
              jb   try          ; - - -
              ja   didnt_go     ; - - -
              cmp  2[BP],CX     ; - - -
              _quif ne          ; - - -
              cmp  0[BP],DX     ; - - -
              je   try          ; - - -
            _endguess           ; - - endguess
            _if   c             ; - - if the carry is set (ie the modulus will
                                ; - - - definitely subtract from the dividend
                                ; - - - without a borrow
try:
              sub  DX,0[BP]     ; - - - subtract divisor from dividend
              sbb  CX,2[BP]     ; - - - . . .
              sbb  BX,4[BP]     ; - - - . . .
              sbb  AL,AH        ; - - - . . .
              stc               ; - - - set carry to indicate that modulus was
                                ; - - - successfully subtracted from dividend
            _endif              ; - - endif
didnt_go:   _rcl  DI,1          ; - - rotate 1 (if carry set) into quotient word
            sub   SI,0010h      ; - - adjust difference in exponents
            jl    _done         ; - - quit if done
            _shl  DX,1          ; - - shift dividend left
            _rcl  CX,1          ; - - . . .
            _rcl  BX,1          ; - - . . .
            _rcl  AL,1          ; - - . . .
            cmp   AL,20h
            jae   try

;<> If bit 5 of dividend is set here, we didnt subtract the modulus from the
;<> dividend (recall that the divisor has a 1 in the msb -- if we subtracted
;<> it from the dividend without a borrow, the dividend would not have a one
;<> in its msb to be shifted into bit 5 tested for in the condition above. If
;<> we are rotating a bit into bit 5, the dividend is now big enough that we
;<> can be sure of subtracting out the divisor without a borrow, as we have
;<> shifted it left one digit.

            cmp   AL,10h
          _until  b             ; - until

          cmc                   ; - flip the carry bit
          jmp   didnt_go        ; - continue
_done:    sub   SI,SI           ; - set SI to 0
;         normalize the remainder in AL:BX:CX:DX
          _guess                ; - guess: number is zero
            cmp   AL,0          ; - - quit if not zero
            _quif ne            ; - - ...
            or    BX,BX         ; - - ...
            _quif ne            ; - - ...
            or    CX,CX         ; - - ...
            _quif ne            ; - - ...
            or    DX,DX         ; - - ...
            _quif ne            ; - - ...
          _admit                ; - admit: not zero
            _loop               ; - - loop
              test  AL,20h      ; - - - quit if number is normalized
              _quif ne          ; - - - . . .
              rcl   DX,1        ; - - - shift result left
              rcl   CX,1
              rcl   BX,1
              rcl   AL,1
              sub   SI,0010h    ; - - - decrement exponent
            _endloop            ; - - endloop
            shr   AL,1          ; - - put in correct position
            rcr   BX,1          ; - - . . .
            rcr   CX,1          ; - - . . .
            rcr   DX,1          ; - - . . .
            add   SI,0010h      ; - - increment exponent
            push  DI            ; - - save quotient
            mov   DI,6[BP]      ; - - get most sig word of op2
            and   DI,7FF0h      ; - - isolate exponent of modulus
            add   SI,DI         ; - - adjust exponent of result
            pop   DI            ; - - restore quotient
          _endguess             ; - endguess
        _else                   ; else
          add   SI,DI           ; - restore exponent
          sub   DI,DI           ; - set quotient to 0
        _endif                  ; endif
        and     AX,000Fh        ; keep just the fraction
        add     AX,SI           ; update high order word
        pop     SI              ; restore sign
        and     SI,08000h       ; isolate sign bit
        or      AX,AX           ; test high word of remainder
        _if     ne              ; if remainder is non-zero
          or    AX,SI           ; - make remainder same sign as original opnd
        _endif                  ; endif
        xor     SI,6[BP]        ; calc sign of quotient
        _if     s               ; if quotient should be negative
          neg   DI              ; - negate quotient
        _endif                  ; endif
_return:
        mov     SI,[BP+10]      ; store address of remainder
        mov     BP,[BP+8]       ; get address of quotient
        mov     [BP],DI         ; store quotient
        mov     BP,SI           ; get address of remainder
        mov     [BP],DX         ; store remainder
        mov     [BP+2],CX       ; ...
        mov     [BP+4],BX       ; ...
        mov     [BP+6],AX       ; ...
        pop     BX              ; restore BX
        pop     CX              ; restore CX
        pop     DX              ; restore DX
        pop     DI              ; restore DI
        pop     SI              ; restore SI
        pop     BP              ; restore BP
        ret                     ; return
        endproc __fprem_


        endmod
        end
