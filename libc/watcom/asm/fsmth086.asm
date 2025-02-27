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
;     real*4 math library
;
;  04-apr-86    G. Coschi       special over/underflow check in mul,div
;                               have to always point at DGROUP.
;                               we might be running with SS != DGROUP
;
;     inputs: DX,AX - operand 1 (high word, low word resp. ) (op1)
;             CX,BX - operand 2                              (op2)
;
;     operations are performed as op1 (*) op2 where (*) is the selected
;     operation
;
;     output: DX,AX - result    (high word, low word resp. )
;
;     __FSA, __FSS - written  28-apr-84
;                  - modified by A.Kasapi 15-may-84
;                  - to:      Calculate sign of result
;                  -          Guard bit in addition for extra accuracy
;                             Add documentation
;     __FSM        - written  16-may-84
;                  - by       Athos Kasapi
;     __FSD        - written  may-84 by "
;
;
;
include mdef.inc
include struct.inc

.8087
        modstart        fsmth086

        xrefp           __8087  ; indicate that NDP instructions are present

        xrefp   F4DivZero       ; Fstatus
        xrefp   F4OverFlow      ; Fstatus
        xrefp   F4UnderFlow     ; Fstatus
        xrefp   __fdiv_m32

go_to   macro frtn
if _MODEL and _DS_PEGGED
        jmp     frtn
else
 if _MODEL and (_BIG_DATA or _HUGE_DATA)
        push    ax
        push    bp
        push    ds
        mov     ax,DGROUP               ; get access to DGROUP
        mov     bp,sp
        mov     ds,ax                   ; . . .
        mov     ax,frtn
        xchg    ax,4[bp]
        pop     ds
        pop     bp
        retn
 else
        jmp     frtn
 endif
endif
endm

        datasegment
        extrn   __real87 : byte         ; cstart
        extern  __chipbug : byte
fsadd   dw      _chkadd
fsmul   dw      _chkmul
fsdiv   dw      _chkdiv
        enddata

        xdefp   __FSA           ; add real*4 to real*4
        xdefp   __FSS           ; subtract real*4 from real*4
        xdefp   __FSM           ; 4-byte real multiply
        xdefp   __FSD           ; 4-byte real divide


        defpe   __FSS
        jcxz    ret_op1         ; if op2 is 0 then return operand 1
        xor     CH,80h          ; flip the sign of op2 and add

        defpe   __FSA
        jcxz    ret_op1         ; if op2 is 0 then return operand 1
        or      DX,DX           ; if op1 is 0
        _if     e               ; then
          mov   DX,CX           ; - return operand 2
          mov   AX,BX           ; - . . .
ret_op1:  ret                   ; - return
        _endif                  ; endif
        go_to   fsadd

__FSA87:
        push    BP              ; save BP
        mov     BP,SP           ; get access to stack
        push    DX              ; push operand 1
        push    AX              ; . . .
        fld     dword ptr -4[BP]; load operand 1
        push    CX              ; push operand 2
        push    BX              ; . . .
        fadd    dword ptr -8[BP]; add operand 2 to operand 1
_ret87:
        fstp    dword ptr -4[BP]; store result
        add     sp,4            ; clean up stack
        fwait                   ; wait
        pop     AX              ; load result into DX:AX
        pop     DX              ; . . .
        cmp     DX,8000h        ; check for negative zero
        _if     e               ; if it is then
          sub     AX,AX         ; - make it positive
          mov     DX,AX         ; - ...
        _endif                  ; endif
        pop     BP              ; restore BP
        ret                     ; return

__FSAemu:
        push    DI              ; save DI
        xchg    AX,BX           ; flip registers around
        xchg    AX,DX           ; . . .
;
;       now have:
;               AX:BX
;           +-  CX:DX
;<> Scheme for calculating sign of result:
;<>   The sign word is built and kept in DL
;<>   Bits 0 and 1 hold the sum of the sign bits
;<>       shifted out of op_1 and op_2
;<>   Bit 2 holds the sign of the larger operand. It is assumed to be
;<>       op_1 until op_2 is found larger

        mov     DI,DX           ; put low order word of op2 in DI
        sub     DX,DX           ; clear DX
        _shl    AX,1            ; get exponent of op1 into AH
        _rcl    DL,1            ;
        mov     DH,DL           ;
        _shl    DL,1            ;
        _shl    DL,1            ;
        add     DL,DH           ;
        stc                     ; put implied 1 bit into top bit of
        rcr     AL,1            ; ... fraction
        _shl    CX,1            ; get exponent of op2 into CH
        adc     DL,0            ;
        stc                     ; put implied 1 bit into top bit
        rcr     CL,1            ; ... of fraction
        mov     DH,AH           ; assume op1 > op2
        sub     AH,CH           ; calculate difference in exponents
        _if     ne              ; if different
          _if   b               ; - if op1 < op2
            mov   DH,CH         ; - - get larger exponent for result
            neg   AH            ; - - negate the shift count
            xchg  AL,CL         ; - - flip operands
            xchg  BX,DI         ; - - . . .
            xor   DL,4

;<> op_2 is larger, so its sign now occupies bit 2 of sign word.  This
;<> information is only correct if the signs of op-1 and op-2 are different.
;<> Since we look past bit 1 for sign only if the signs are different, bit2
;<> will supply the correct information when it is needed. We get the sign of
;<> op_2 by flipping the sign of op_1, already in bit 2

          _endif                ; - endif
          mov   CH,AH           ; - get shift count
          xor   AH,AH           ; - zero guard byte
          _loop                 ; - loop (align fractions)
            shr   CL,1          ; - - shift over fraction
            rcr   DI,1          ; - - . . .
            dec   CH            ; - - decrement shift count
          _until e              ; - until fractions aligned
          rcr   AH,1            ; - only need last bit in guard byte

;<> bit 7 of the guard byte holds an extra significant bit from mantissa
;<> of the operand we shifted left to allign with the greater operand
;<> it is added or subtracted from with operands by shifting the bit into
;<> the carry bit just before the operation

        _endif                  ; endif
        shr     DL,1            ; get bit 0 of sign word - value is 0 if
                                ; both operands have same sign, 1 if not
        _if     nc              ; if signs are the same
          add   BX,DI           ; - add the fractions
          adc   AL,CL           ; - . . .
          _if   c               ; - if carry
            rcr   AL,1          ; - - shift fraction right 1 bit
            rcr   BX,1          ; - - . . .
            rcr   AH,1          ; - - save extra sig bit in guard bit
            inc   DH            ; - - increment exponent
            _if   z             ; - - if we overflowed
              ror dl,1          ; - - - set sign of infinity
              rcr ah,1          ; - - - . . .
              jmp add_oflow     ; - - - handle overflow
            _endif              ; - - endif
          _endif                ; - endif
        _else                   ; else (signs are different)
          shr   DL,1            ; - skip junk bit
          rol   AH,1            ; - get guard bit
          ror   AH,1            ; - and put it back
          sbb   BX,DI           ; - subtract the fractions
          sbb   AL,CL           ; - . . .
          _guess                ; - guess
            _quif nc            ; - - quit if no borrow
            inc   DL            ; - - sign := sign of op_2
            not   AL            ; - - negate the fraction (considering
                                ; - - the guard bit an extension of the
            not   BX            ; - - fraction)
            neg   AH            ; - - . . .
            sbb   BX,-1         ; - - . . .
            sbb   AL,-1         ; - - . . .
          _admit                ; - admit
            cmp   AL,0          ; - - quit if answer is not 0
            _quif ne            ; - - . . .
            or    BX,BX         ; - - . . .
            _quif ne            ; - - . . .
            sub   AX,AX         ; - - set result to 0
            sub   DX,DX         ; - - . . .
            pop   DI            ; - - restore DI
            ret                 ; - - return (answer is 0)
          _endguess             ; - endguess
        _endif                  ; endif

        ; normalize the fraction
        _shl    AH,1            ; get guard bit
        adc     BX,0            ; round up fraction if required
        adc     AL,0            ; . . .
        _guess  underflow       ; guess
          _quif nc              ; - quit if round up didn't overflow frac
          inc   DH              ; - adjust exponent
        _admit                  ; admit
          _loop                 ; - loop (shift until high bit appears)
            _rcl  BX,1          ; - - shift fraction left
            _rcl  AL,1          ; - - . . .
            _quif c,underflow   ; - - quit if carry has appeared
            dec   DH            ; - - decrement exponent
          _until  e             ; - until underflow
          jmp   add_uflow       ; - handle underflow
        _endguess               ; endguess
        mov     AH,DH           ; get exponent
        ror     DL,1            ; get sign bit
        rcr     AX,1            ; shift it into result
        rcr     BX,1            ; . . .
        mov     DX,AX           ; get result in DX:AX
        mov     AX,BX           ; . . .
        pop     DI              ; restore DI
        ret                     ; return

add_uflow:                      ; handle underflow
        pop     DI              ; restore DI
        jmp     F4UnderFlow     ; goto underflow routine

add_oflow:                      ; handle overflow
        pop     DI              ; restore DI
        jmp     F4OverFlow      ; handle overflow
        endproc __FSA
        endproc __FSS

;=====================================================================

        defpe   __FSM

;<> multiplies X by Y and places result in C.
;<> X2 and X1 represent the high and low words of X. Similarly for Y and C
;<> Special care is taken to use only six registers, so the code is a bit
;<> obscure

        _guess                  ; guess: answer not 0
          or    DX,DX           ; - see if first arg is zero
          _quif e               ; - quit if op1 is 0
          or    CX,CX           ; - quit if op2 is 0
          _quif e               ; - . . .
          go_to fsmul           ; - invoke support rtn
        _endguess               ; endguess
        sub     AX,AX           ; set answer to 0
        sub     DX,DX           ; . . .
        ret                     ; return

__FSM87:
        push    BP              ; save BP
        mov     BP,SP           ; get access to stack
        push    DX              ; push operand 1
        push    AX              ; . . .
        fld     dword ptr -4[BP]; load operand 1
        push    CX              ; push operand 2
        push    BX              ; . . .
        fmul    dword ptr -8[BP]; mulitply operand 1 by operand 2
        jmp     _ret87          ; goto common epilogue

__FSMemu:
        push    SI              ; save SI
        push    DI              ; save DI
        xchg    AX,BX           ; flip registers around
        xchg    AX,DX           ; . . .
;
;       now have:
;               AX:BX
;            *  CX:DX
;
        mov     DI,BX           ; move arguments to better registers
        mov     BX,AX           ; . . .
        mov     SI,DX           ; move X1 to less volatile register

        _shl    BX,1            ; move sign of X
        _rcl    AL,1            ; into AL
        stc                     ; rotate implied high bit into X
        rcr     BL,1            ; . . .
        _shl    CX,1            ; use sign of Y
        adc     AL,0            ; to calculate sign of result in AL
        stc                     ; rotate implied high bit into Y
        rcr     CL,1            ; . . .

        xchg    AL,CH           ; put sign in CH and get exponent of Y
        sub     AL,7Fh          ; remove bias from exponents
        sub     BH,7Fh          ; . . .
        add     BH,AL           ; add exponents
        _if     o               ; if over or underflow
          js    mul_oflow       ; - report overflow if signed
          jmp   mul_uflow       ; - handle underflow
        _endif                  ; endif
        cmp     BH,81h          ; check for underflow
        jle     mul_uflow       ; quit if underflow
        add     BH,7fh+2        ; bias exponent
        xchg    BH,CL           ; put exponent of result in CL and move Y2
                                ; into BH. Note that BL holds X2
        mov     AX,DI           ; get X1

        mul     SI              ; C1 := high_word( X1 * Y1 ) and
        xchg    DX,DI           ; get X1
        mov     AX,BX           ; get Y2
        xchg    AH,AL           ; put factor into lower register
        sub     AH,AH           ; . . .

        mul     DX              ; C2 := high_word( X1 * Y2 ) and
        xchg    DX,SI           ; get Y1
        add     DI,AX           ; (C2_C1) += low_word( X1 * Y2 )
        adc     SI,0            ; . . .
        mov     AX,BX           ; get X2
        sub     AH,AH           ; . . .
        mul     DX              ; (C2_C1) += low_word( X2 * Y1 )
        add     DI,AX           ; . . .
        adc     SI,DX           ; C2 += high_word( X2 * Y1 )
        mov     AX,BX           ; get X2
        mul     AH              ; C2 += byte_product( Y2 * X2 )
        add     SI,AX           ; . . .

        _loop                   ; loop
          _shl  DI,1            ;   shift result left
          _rcl  SI,1            ;   . . .
          dec   CL              ;   and dec exponent for every shift
        _until  be              ; until( carry flag or zero flag is set )
        jz      mul_oflow       ; ...

        mov     AX,SI           ; move result to more flexible registers
        mov     BX,DI           ; . . .
        mov     BL,BH           ; shift exponent into result
        mov     BH,AL           ; . . .
        mov     AL,AH           ; . . .
        mov     AH,CL           ; . . .
        add     BX,1            ; round up fraction
        adc     AX,0            ; and increment exponent if necessary
        jz      mul_oflow       ; report overflow if required
        shr     CH,1            ; shift sign into result
        rcr     AX,1            ; . . .
        rcr     BX,1            ; . . .
        mov     DX,AX           ; get result into DX:AX
        mov     AX,BX           ; . . .
        pop     DI              ; restore DI
        pop     SI              ; restore SI
        ret                     ; return

mul_uflow:                      ; underflow
        pop     DI              ; restore DI
        pop     SI              ; restore SI
        jmp     F4UnderFlow     ; . . .

mul_oflow:                      ; overflow
        shr     CH,1            ; get sign of infinity
        rcr     AX,1            ; into proper register
        pop     DI              ; restore DI
        pop     SI              ; restore SI
        jmp     F4OverFlow      ; report overflow
        endproc __FSM

;====================================================================

        defpe   __FSD
        go_to   fsdiv

__FSDbad_div:
        push    BP              ; save BP
        mov     BP,SP           ; get access to stack
        push    DX              ; push operand 1
        push    AX              ; . . .
        fld     dword ptr -4[BP]; load operand 1
        push    CX              ; push operand 2
        push    BX              ; . . .
        call    __fdiv_m32      ; divide operand 1 by operand 2
        sub     sp,4            ; rtn pops parm, _ret87 wants it
        jmp     _ret87          ; goto common epilogue

__FSD87:
        push    BP              ; save BP
        mov     BP,SP           ; get access to stack
        push    DX              ; push operand 1
        push    AX              ; . . .
        fld     dword ptr -4[BP]; load operand 1
        push    CX              ; push operand 2
        push    BX              ; . . .
        fdiv    dword ptr -8[BP]; divide operand 1 by operand 2
        jmp     _ret87          ; goto common epilogue

__FSDemu:
        _shl    CX,1            ; shift sign of divisor into carry
        _if     e               ; if divisor is zero
          jmp   F4DivZero       ; - handle divide by zero
        _endif                  ; endif
        push    SI              ; save SI
        push    DI              ; save DI
        xchg    AX,BX           ; flip registers around
        xchg    AX,DX           ; . . .
;
;       now have:
;               AX:BX
;               -----
;               CX:DX
;
        mov     DI,DX           ; save DX in DI
                                ; CX:DI is the divisor;  AX:BX is the dividend
        _rcl    DL,1            ; save sign in DL
        or      AX,AX           ; check dividend for zero
        _if     e               ; if so then
          sub   DX,DX           ; - make sure both parts are 0
          pop   DI              ; - restore DI
          pop   SI              ; - restore SI
          ret                   ; - return
        _endif                  ; endif
        stc                     ; rotate implied '1'bit back into divisor
        rcr     CL,1            ; . . .
        _shl    AX,1            ; shift sign of divisor into carry
        adc     DL,0            ; now calculate save sign of result in DL
        stc                     ; rotate implied '1' bit into dividend
        rcr     AL,1            ; . . .
        sub     CH,7Fh          ; calculate exponent of result
        sub     AH,7Fh          ; . . .
        sub     AH,CH           ; . . .
        _if     o               ; if over or underflow
          _if   s               ; - if overflow
            shr dl,1            ; - - get sign of infinity
            rcr ax,1            ; - - . . .
            jmp div_oflow       ; - - handle overflow
          _endif                ; - else
          jmp   div_uflow       ; - - handle underflow
        _endif                  ; endif
        cmp     AH,81h          ; check for underflow
        jle     div_uflow       ; . . .
        add     AH,7Fh          ; restore bias to exponent
        mov     DH,AH           ; save calculated exponent
        mov     CH,25
        mov     AH,CL

; The count is set to 25.  We do not know if the first digit of our quotient
; mantissa will be a one or zero.  We do know that if the first calculated
; digit was not a 1, the second one will be, since we will have by then
; shifted the dividend left and made it large enough for the divisor to
; subtract out of  To always calculate 24 SIGNIFICANT digits (ie with a leading
; one) in the mantissa we must, then, calculate 25, in case the leading digit
; was a zero.

        _loop                   ; loop
          _guess                ; - The purpose of this guess is to set the
            cmp AH,AL           ; - Carry bit by the time we reach endguess
            jl  try
            jg  didnt_go
            cmp DI,BX           ; - . . .
            je  try             ; - . . .
          _endguess             ; - . . .
          _if   c               ; - if
                                ; - - the carry is set (ie the divisor will
                                ; - - definitely subract from the dividend
                                ; - - without a borrow
try:        sub BX,DI           ; - - subtract the divisor from dividend
            sbb AL,AH           ; - - . . .
            stc                 ; - - set cary, to indicate that the divisor
                                ; - - was subtracted from the dividend
          _endif                ; - endif
didnt_go: _rcl  SI,1            ; - rotate a 1 into quotient if carry set
          _rcl  CL,1            ; - . . .
          dec   CH              ; - count --
          jle   done            ; - if( count = 0 ) goto done
resume:
          _shl  BX,1            ; - shift divisor left
          _rcl  AL,1            ; - . . .
          jc    try

; If the carry is set here, we didnt subtract the divisor from the dividend
; (recall that the divisor has a 1 in the msb -- if we subtracted it from
; the dividend without a borrow, the dividend would not have a one in
; its msb to be shifted into the carry tested for in the condition above.
; If we are rotating a carry out of the dividend, the dividend
; is now big enough that we can be sure of subtracting out the divisor
; without a borrow, as we have shifted it left one digit.

          jno   didnt_go

; If the overflow is not set and the carry is not set, we know there is
; now a '0' in the msb of the dividend and since there is a '1' in the
; msb of the divisor, we know that it wont subtract from the dividend
; without a borrow, so we dont even check --- just jump to a place where
; we can shift a 0 into the quotient for the current digit

        _endloop
done:

; The following conditional ensures that not only do we have 24 significant
; normalized digits in our quotient registers, but we also have the next
; (would have followed the lsb) bit in the carry.  If we do not have a
; carry at this point, we only have 24 sig digits.  We can use the carry
; to hold an extra significant digit, which we go back for in the conditional

        _if     nc
          dec   DH

; the exponent is decremented because in going back for an extra digit, we
; shift the quotient left one bit, and hence multiply it by two.

          jnz   resume          ; on not overflow, goto resume
          jmp   div_uflow       ; handle underflow
        _endif

; we know that the carry is set here --- ie we have shifted
; a significant bit of the quotient into the carry in order
; to make room for an extra significant bit in the lsb position
; of the word. We now put the msb in the carry back into the msb
; of the word, and use the lsb shifted into the carry in doing this to
; round off the quotient. We do this by rotating right the carry into the
; quotient and saving the least sig bit in the carry. This is added back on to
; round off the number. we do not alter the exponent when we right, because
; we shifted left 25 times instead of 24, so the quotent needed right shifting

        rcr     CL,1
        rcr     SI,1
        adc     SI,0            ; add back lsb to round off quotient
        adc     CL,0            ; . . .
        _guess                  ; guess have to inc exponent
          _quif nc              ; - quit if no carry
          inc   dh              ; - increment exponent
          _quif nz              ; - quit if no overflow
          shr   dl,1            ; - get sign of infinity
          rcr   ax,1            ; - . . .
          jmp   div_oflow       ; - handle overflow
        _endguess               ; endguess

        ror     DX,1            ; rotate sign bit into high bit
        or      DX,007Fh        ; prepare sign-exponent word
        mov     CH,0FFh         ; prepare high word of mantissa word
        and     CX,DX           ; mask the sign and exponent into quotient
        mov     DX,CX           ; move high order word of quotent to DX
        mov     AX,SI           ; move low  ...    ...        ...    AX
        pop     DI              ; restore DI
        pop     SI              ; restore SI
        ret                     ; return to caller

div_uflow:                      ; handle underflow
        pop     DI              ; restore DI
        pop     SI              ; restore SI
        jmp     F4UnderFlow     ; handle underflow


div_oflow:                      ; handle overflow
        pop     DI              ; restore DI
        pop     SI              ; restore SI
        jmp     F4OverFlow      ; handle overflow
        endproc __FSD


_chkadd: call   _chk8087
        go_to   fsadd

_chkmul: call   _chk8087
        go_to   fsmul

_chkdiv: call   _chk8087
        go_to   fsdiv

_chk8087 proc   near
        push    ax                      ; save AX
if (_MODEL and _DS_PEGGED) eq 0
if _MODEL and (_BIG_DATA or _HUGE_DATA)
        push    ds                      ; save DS
        mov     ax,DGROUP               ; get access to DGROUP
        mov     ds,ax                   ; . . .
endif
endif
        cmp     byte ptr __real87,0     ; if 8087 present
        _if     ne                      ; then
          mov   ax,offset __FSA87       ; - get addr of add rtn
          mov   fsadd,ax                ; - ...
          mov   ax,offset __FSM87       ; - get addr of mul rtn
          mov   fsmul,ax                ; - ...
          test  byte ptr __chipbug,1    ; - if we have a bad divider
          _if   ne                      ; - then
            mov ax,offset __FSDbad_div  ; - - get addr of div rtn
          _else                         ; - else
            mov ax,offset __FSD87       ; - - get addr of div rtn
          _endif
          mov   fsdiv,ax                ; - ...
        _else                           ; else
          mov   ax,offset __FSAemu      ; - get addr of add rtn
          mov   fsadd,ax                ; - ...
          mov   ax,offset __FSMemu      ; - get addr of mul rtn
          mov   fsmul,ax                ; - ...
          mov   ax,offset __FSDemu      ; - get addr of div rtn
          mov   fsdiv,ax                ; - ...
        _endif                          ; endif
if (_MODEL and _DS_PEGGED) eq 0
if _MODEL and (_BIG_DATA or _HUGE_DATA)
        pop     ds                      ; restore ds
endif
endif
        pop     ax                      ; restore AX
        ret                             ; return
        endproc _chk8087

        endmod
        end
