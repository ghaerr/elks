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
;* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
;*               DESCRIBE IT HERE!
;*
;*****************************************************************************


;
;   real*8 math library
;
;   __FDM,__FDD
;                   floating point routines
;                   13 June, 1984 @Watcom
;
;   All routines have the same calling conventions.
;   Op_1 and Op_2 are double prec reals, pointed to by DI and SI resp.
;   The binary operations are perfomed as Op_1 (*) Op_2.
;
;   In all cases, BP and DI are returned unaltered.
;
;
;                               have to always point at DGROUP.
;                               **** This routine does CODE modification ****
;                               is a power of 2.
;                               aligning fractions before the add
;                               No need to push second operand in f8split
;                               since ss:[si] can already be used to access it
;                               Moved f8split into each subroutine.
;                               to get rid of code modification
;                               we might be running with SS != DGROUP
;
include mdef.inc
include struct.inc

.8087
        modstart        fdmth086

        xrefp           __8087  ; indicate that NDP instructions are present

        xrefp   F8OverFlow              ; FSTATUS
        xrefp   F8UnderFlow             ; FSTATUS
        xrefp   F8DivZero               ; FSTATUS
        xrefp   __fdiv_m64r

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

;
        datasegment
        extrn   __real87 : byte         ; cstart
        extrn  __chipbug : byte
fdadd   dw      _chkadd
fdsub   dw      _chksub
fdmul   dw      _chkmul
fddiv   dw      _chkdiv
        enddata

        xdefp   __FDA
        xdefp   __EDA
        xdefp   __FDS
        xdefp   __EDS
        xdefp   __FDM
        xdefp   __EDM
        xdefp   __FDD
        xdefp   __EDD

        defp    __EDD
        push    es:6[si]
        push    es:4[si]
        push    es:2[si]
        push    es:[si]
        mov     si,sp           ; point to second operand
        docall  __FDD
        add     sp,8
        ret
        endproc __EDD

        defp    __EDM
        push    es:6[si]
        push    es:4[si]
        push    es:2[si]
        push    es:[si]
        mov     si,sp           ; point to second operand
        docall  __FDM
        add     sp,8
        ret
        endproc __EDM

        defp    __EDS
        push    es:6[si]
        push    es:4[si]
        push    es:2[si]
        push    es:[si]
        mov     si,sp           ; point to second operand
        docall  __FDS
        add     sp,8
        ret
        endproc __EDS

        defp    __EDA
        push    es:6[si]
        push    es:4[si]
        push    es:2[si]
        push    es:[si]
        mov     si,sp           ; point to second operand
        docall  __FDA
        add     sp,8
        ret
        endproc __EDA


;
; __FDS
;

        defpe   __FDS
        go_to   fdsub

__FDS87:
        fld     qword ptr ss:[SI] ; load operand 2
        push    BP              ; save BP
        mov     BP,SP           ; get access to stack
        push    AX              ; push operand 1
        push    BX              ; . . .
        push    CX              ; . . .
        push    DX              ; . . .
        fsubr   qword ptr -8[BP]; subtract operand 2 from operand 1
_ret87: fstp    qword ptr -8[BP]; store result
        fwait                   ; wait
        pop     DX              ; load result into AX:BX:CX:DX
        pop     CX              ; . . .
        pop     BX              ; . . .
        pop     AX              ; . . .
        cmp     AX,8000h        ; is it negative zero?          17-mar-91
        _if     e               ; if it is then
          sub   AX,AX           ; - turn it into positive zero
          mov   BX,AX           ; - ... zero other words as well
          mov   CX,AX           ; - ...
          mov   DX,AX           ; - ...
        _endif                  ; endif
        pop     BP              ; restore BP
        ret                     ; return

__FDSemu:
        push    BP              ; save BP
        mov     BP,8000h        ; indicate that we are doing subtraction
        jmp     begin
        endproc __FDS


;
; __FDA
;


        defpe   __FDA
        go_to   fdadd

__FDA87:
        fld     qword ptr ss:[SI] ; load operand 2
        push    BP              ; save BP
        mov     BP,SP           ; get access to stack
        push    AX              ; push operand 1
        push    BX              ; . . .
        push    CX              ; . . .
        push    DX              ; . . .
        fadd    qword ptr -8[BP]; add operand 1
        jmp     _ret87          ; return result from 8087

retOp_2:
        sub     bx,bx           ; return op2
        _shl    cx,1            ; get sign bit into position
        mov     bp,cx           ; move it to correct reg
        xor     bp,ss:6[si]     ; see if sign must be reversed
        and     bp,8000h        ; clear out rest of reg
                                ; one of the args was found to be zero
addzero:or      BX,BX           ; check the first argument
        _if     ne              ; if the first arg is not zero
          mov   SI,DI           ; - return first argument
          sub   bp,bp           ; - don't reverse sign
        _endif                  ; endif
        mov     ax,ss:6[si]     ; get high word
        or      ax,ax           ; see if number is zero
        _if     ne              ; if not then
          xor   ax,bp           ; - reverse sign if required
        _endif                  ; endif
        mov     BX,ss:4[SI]     ; get next sig word
        mov     CX,ss:2[SI]     ; . . .
        mov     DX,ss:[SI]      ; . . .
        add     SP,8            ; clean up stack
        pop     DI              ; restore DI
        pop     BP              ; return to caller
        ret

__FDAemu:
        push    BP
        sub     BP,BP           ; indicate that we are doing addition
begin:  push    DI              ; save DI

;       copy of the f8split routine

        push    AX              ; push operand 1 onto stack
        push    BX              ; . . .
        push    CX              ; . . .
        push    DX              ; . . .
        mov     DI,SP           ; get address of operand 1
        _guess                  ; guess
          mov   BX,AX           ; - get most sig word of op1
          mov   AX,ss:6[SI]     ; - get most sig word of op2
          mov   DL,AL           ; - save mantissa-holding part
          mov   DH,BL           ; - save mantissa-holding part
          mov   cl,bh           ; - save sign
          and   BX,7FF0h        ; - isolate exponent
          je    addzero         ; - quif zero
          mov   ch,ah           ; - save sign
          and   AX,7FF0h        ; - isolate exponent
          je    addzero         ; - quif zero
          and   cx,8080h        ; - only want sign bits
          xor   dx,1010h        ; - set implied one bit of mant.
          xor   DL,AL           ; - . . .
          xor   DH,BL           ; - . . .
        _endguess               ; endguess

;       end of f8split

        xor     CX,BP           ; change sign of op_2 if this is sub
        sar     CH,1            ; prepare sign byte
        add     CH,CL           ; it is op_2's sign we indicate
        mov     BP,AX           ; save exponent
        sub     AX,BX           ; get shift count
        _if     l               ; - if op_2 < op_1
          mov   BP,BX           ; - - move op_1's exponent into BP
          neg   AX              ; - - negate the shift count
          xchg  SI,DI           ; - - exchange operands
          xchg  DH,DL           ; - - . . .
          _shl  CH,1            ; - - make op_1's sign count
          rcr   CL,1
          xchg  CL,CH           ; - - . . .
        _endif                  ; - endif
        cmp     AX,0400h        ; AX has shift count << 4
        jae     retOp_2         ; if shift count >= 64, return operand 2
        shr     AX,1            ; move shift count into bottom bits
        shr     AX,1
        shr     AX,1
        shr     AX,1
        mov     AH,AL           ; AH := shift count
        mov     AL,DH           ; AL := high byte of smaller real
        mov     DH,CH           ; DH := sign byte
        mov     BX,ss:4[DI]     ; . . .
        mov     CX,ss:2[DI]     ; . . .
        mov     DI,ss:[DI]      ; . . .
        or      AH,AH           ; test AH (shift count)
        _if     ne              ; if shift count <> 0
          xchg  DX,DI           ; - get DI into byte registers for shift
          _loop                 ; - loop (shift by a byte at a time)
            cmp   AH,8          ; - - quit if less than a byte to do
            _quif l             ; - - . . .
            sub   AH,8          ; - - subtract 8 from shift count
            _if   e             ; - - if exact # of bytes
              mov   AH,DL       ; - - - get high bit for guard bit
            _endif              ; - - endif
            mov   DL,DH         ; - - shift right 1 byte
            mov   DH,CL         ; - - . . .
            mov   CL,CH         ; - - . . .
            mov   CH,BL         ; - - . . .
            mov   BL,BH         ; - - . . .
            mov   BH,AL         ; - - . . .
            mov   AL,0          ; - - . . .
          _until  e             ; - until done
          _if   ne              ; - if still more to shift
            _loop               ; - - loop
              shr   AL,1        ; - - - shift over fractions
              rcr   BX,1        ; - - - . . .
              rcr   CX,1        ; - - - . . .
              rcr   DX,1        ; - - - . . .
              dec   AH          ; - - - dec shift count
            _until  e           ; - - until shift count = 0
            rcr   AH,1          ; - - save guard bit
          _endif                ; - endif
          and   AH,80h          ; - isolate guard bit
          xchg  DX,DI           ; - exchange regs again
        _endif                  ; endif
        _shl    DH,1            ; get bit 7 of sign word. 1 if signs are
                                ; different, 0 if they are the same
        _if     nc              ; if signs are the same
          add   DI,ss:[SI]      ; - add the mantissas
          adc   CX,ss:2[SI]     ; - . . .
          adc   BX,ss:4[SI]     ; - . . .
          adc   AL,DL           ; - . . .
          test  AL,20h          ; - check for carry
          je    add_norm        ; - if we haven't carried, normalize now
          push  DX              ; - put sign onto stack
          mov   DX,DI           ; - get low word into correct reg
          jmp   fin_up          ; - common finish up routine
        _endif                  ; endif
        sub     DI,ss:[SI]
        sbb     CX,ss:2[SI]     ; . . .
        sbb     BX,ss:4[SI]     ; . . .
        sbb     AL,DL           ; . . .
        cmc                     ; complement carry
        rcr     SI,1            ; save complemented carry in SI
        and     SI,8000h        ; isolate the bit
        add     DX,SI           ; and use it to calculate sign of result
        _shl    SI,1            ; restore the complement of the carry
        _if     c               ; if no borrow, see if result is zero
          cmp   AL,0            ; - - quif answer is not zero
          jne   add_norm        ; - - . . .
          or    SI,BX
          or    SI,CX
          or    SI,DI
          jne   add_norm
          add   SP,8            ; - clean up stack
          pop   DI              ; - restore DI
          pop   BP              ; - restore bp
          sub   AX,AX           ; - set answer to 0
          sub   BX,BX           ; - . . .
          sub   CX,CX           ; - . . .
          sub   DX,DX           ; - . . .
          ret                   ; - return
        _endif
        neg     DH              ; - sign is that of other operand
        not     AL              ; - negate the fraction
        not     BX              ; - . . .
        not     CX              ; - . . .
        not     DI              ; - . . .
        neg     AH              ; - (considering the guard bit as an extension)
        sbb     DI,-1           ; - . . .
        sbb     CX,-1           ; - . . .
        sbb     BX,-1           ; - . . .
        sbb     AL,-1           ; - . . .

add_norm:                       ; normalizes the mantissa against bit 6
        add     BP,0010h        ; prepare exponent

;<> please note that this will overflow if the exponent is very close to its
;<> maximum size but not too large to overflow under normal circumstances.
;<> since we allow an 11 bit exponent, then, the largest exponent one can
;<> handle with this routine is (10^) 1022.

        _shl    AH,1            ; get guard bit
        _loop                   ; loop
          _rcl  DI,1            ; - shift result left
          _rcl  CX,1
          _rcl  BX,1
          _rcl  AL,1
          sub   BP,0010h        ; - decrement exponent
          jbe   add_uflow       ; - ******* handle underflow *******
          test  AL,20h          ; until msb is in bit 6
        _until ne
        and     AL,1Fh          ; turn off implied one bit
        push    DX              ; push sign
        mov     DX,DI
        jmp     fin_up

; over/underflow entry points for __FDA and __FDS

add_oflow:
        mov     AX,DX           ; put sign into ax
        add     SP,8            ; clean up stack
        pop     DI              ; restore DI
        pop     BP              ; restore bp
        jmp     F8OverFlow      ; handle overflow

add_uflow:
        add     SP,8            ; clean up stack
        pop     DI              ; restore DI
        pop     BP              ; restore bp
        jmp     F8UnderFlow     ; handle underflow
        endproc __FDA

;
; __FDM
;
; Note that if the real pointed to by DI has few sig digits,
; a short cut is taken.
;
        defpe   __FDM
        go_to   fdmul

__FDM87:
        fld     qword ptr ss:[SI] ; load operand 2
        push    BP              ; save BP
        mov     BP,SP           ; get access to stack
        push    AX              ; push operand 1
        push    BX              ; . . .
        push    CX              ; . . .
        push    DX              ; . . .
        fmul    qword ptr -8[BP]; - multiply by operand 1
        jmp     _ret87          ; goto common epilogue


__FDMemu:
        push    BP              ; save BP
        push    DI              ; save DI

;       f8split

        push    AX              ; push operand 1 onto stack
        push    BX              ; . . .
        push    CX              ; . . .
        push    DX              ; . . .
        mov     DI,SP           ; get address of operand 1
        _guess                  ; guess
          mov   BX,AX           ; - get most sig word of op1
          mov   AX,ss:6[SI]     ; - get most sig word of op2
          mov   DL,AL           ; - save mantissa-holding part
          mov   DH,BL           ; - save mantissa-holding part
          mov   cl,bh           ; - save sign
          and   BX,7FF0h        ; - isolate exponent
          _quif e               ; - quif zero
          mov   ch,ah           ; - save sign
          and   AX,7FF0h        ; - isolate exponent
          _quif e               ; - quif zero
          and   cx,8080h        ; - only want sign bits
          xor   dx,1010h        ; - set implied one bit of mant.
          xor   DL,AL           ; - . . .
          xor   DH,BL           ; - . . .
        _admit                  ; admit: one operand is zero
          add   SP,8            ; - clean up stack
          pop   DI              ; - restore DI
          pop   BP              ; - restore bp
          sub   AX,AX           ; - set result to 0
          sub   BX,BX           ; - . . .
          sub   CX,CX           ; - . . .
          sub   DX,DX           ; - . . .
          ret                   ; - return
        _endguess               ; endguess
        add     CH,CL           ; determine sign of result
        _guess                  ; guess: OK
          add   AX,BX           ; - determine exponent of result
          sub   ax,3ff0h        ; - remove extra bias
          _quif e               ; - FP underflow if exponent = 0
          cmp   ax,0c000h       ; - if exponent >= $c000
          _quif ae              ; - then FP underflow
        _admit                  ; admit: underflow
          add   SP,8            ; - clean up stack
          pop   DI              ; - restore DI
          pop   BP              ; - restore bp
          jmp   F8UnderFlow
        _endguess               ; endguess
        cmp     ax,7ff0h        ; if $7ff0 >= exponent > $c000
        _if     ae              ; then FP overflow
          mov   AX,CX           ; - put sign into ax
          add   SP,8            ; - clean up stack
          pop   DI              ; - restore DI
          pop   BP              ; - restore bp
          jmp   F8OverFlow      ; - handle overflow
        _endif                  ; endif
        push    CX              ; push sign
        push    AX              ; push exponent

        mov     BX,ss:[DI]      ; get low order word of op_1
        or      BX,BX           ; if it is zero
        _if     e               ; then
          xchg  SI,DI           ; - flip op_1 and op_2 around
          xchg  DL,DH           ; - . . .
        _endif                  ; endif
        sub     AX,AX           ; clear out AX
        mov     AL,DH           ; set AX up with high order mant of op_1
        push    AX              ; save high order mant
        push    ss:4[DI]        ; save rest of op_1
        push    ss:2[DI]        ; . . .
        push    ss:[DI]         ; . . .
        sub     BX,BX           ; clear out a word
        push    BX              ; move two clear words onto stack
        push    BX
        mov     BP,SP           ; BP points at empty word and op_1
        mov     AL,DL           ; set AX up with high order mant of op_2
        push    AX              ; save high order mant
        push    ss:4[SI]        ; save rest of op_2
        push    ss:2[SI]        ; . . .
        mov     BX,ss:[SI]      ; . . .
        _guess                  ; guess: multiplier has lots of 0's in it
          mov   CX,0300h        ; - set loop count
          or    BX,BX           ; - check multiplier
          _quif ne              ; - quit if not zero
          pop   BX              ; - get next word of multiplier
          mov   CH,2            ; - set loop count
          or    BX,BX           ; - check multiplier
          _quif ne              ; - quit if not zero
          pop   BX              ; - get next word of multiplier
          mov   CH,1            ; - set loop count
        _endguess               ; endguess

; result will be kept in CL:DI:SI:2[BP]:0[BP]

        sub     SI,SI           ; clear out SI
        mov     DI,SI           ; clear out DI
        or      BX,BX           ; if low order multiplier non-zero
        _if     ne              ; then
          mov   AX,4[BP]        ; - get low word of op_1
          mul   BX              ; - multiply low word by A
          mov   0[BP],DX        ; - save high order word of result
          mov   AX,6[BP]        ; - get next lowest word of op_1
          mul   BX              ; - multiply this word by A
          add   0[BP],AX        ; - add product onto result
          adc   DI,DX           ; - . . .

          mov   AX,8[BP]        ; - get next lowest word of op_1
          mul   BX              ; - multiply this word by A
          add   DI,AX           ; - add product onto result
          adc   SI,DX           ; - . . .
          mov   2[BP],DI        ; - save result
          sub   DI,DI           ; - set back to 0

          mov   AX,10[BP]       ; - get highest word of op_1
          mul   BX              ; - multiply this word by A
          add   SI,AX           ; - add product onto result
          adc   DI,DX           ; - . . .
        _endif                  ; endif

        _loop                   ; loop
          dec   CH              ; - decrement loop count
          pop   BX              ; - lowest word of op_1 not used so far
          or    BX,BX           ; - check it (call it A) for zero
          je    shift_16        ; - skip and shift result right if 0
          mov   AX,4[BP]        ; - get low word of op_1
          mul   BX              ; - multiply low word by A
          add    [BP],AX        ; - add product onto result
          adc   2[BP],DX        ; - . . .
          adc   SI,0            ; - carry through as neccesary
          adc   DI,0            ; - . . .

          mov   AX,6[BP]        ; - get next lowest word of op_1
          mul   BX              ; - multiply this word by A
          add   2[BP],AX        ; - add product onto result
          adc   SI,DX           ; - . . .
          adc   DI,0            ; - carry through as necessary

          mov   AX,8[BP]        ; - get next lowest word of op_1
          mul   BX              ; - multiply this word by A
          add   SI,AX           ; - add product onto result
          adc   DI,DX           ; - . . .
          adc   CL,0            ; - carry through as necessary

          mov   AX,10[BP]       ; - get highest word of op_1
          cmp   CH,0            ; - check loop count
          _quif e               ; - quit if muliplied 4 times

          mul   BX              ; - multiply this word by A
          add   DI,AX           ; - add product onto result
          adc   CL,DL           ; - . . .

shift_16:
          mov   AX,2[BP]        ; - shift result right by 1 word
          mov    [BP],AX        ; - . . .
          mov   2[BP],SI        ; - . . .
          mov   SI,DI           ; - . . .
          mov   DI,CX           ; - . . .
          and   DI,00FFh        ; (we only move low word)
          xor   CL,CL           ; - . . .
        _endloop                ; endloop
        mul     BL              ; mulitply 2 most significant bytes
        add     AX,DI           ; and add to the result

        pop     DX              ; get low order words of result
        pop     CX              ; . . .
        mov     BX,SI           ; . . .

        add     SP,8            ; remove operand from stack

        shr     AX,1            ; - shift result right 3 times
        rcr     BX,1            ; - . . .
        rcr     CX,1            ; - . . .
        rcr     DX,1            ; - . . .

        shr     AX,1            ; - . . .
        rcr     BX,1            ; - . . .
        rcr     CX,1            ; - . . .
        rcr     DX,1            ; - . . .

        shr     AX,1            ; - . . .
        rcr     BX,1            ; - . . .
        rcr     CX,1            ; - . . .
        rcr     DX,1            ; - . . .

        pop     BP              ; get exponent

        test    AL,40h          ; find out how many sig bits we got
        _if     ne
          add   BP,0010h        ; - increment exponent
          shr   AL,1            ; - move bits to correct pos in words
          rcr   BX,1            ; - . . .
          rcr   CX,1            ; - . . .
          rcr   DX,1            ; - . . .
        _endif                  ; endif
        and     AL,1Fh          ; clear out implied '1' bit
        jmp     fin_up          ; go to general finish up routine
        endproc __FDM


;
; __FDD
;

        defpe   __FDD
        go_to   fddiv

__FDDbad_div:
        fld     qword ptr ss:[SI] ; load operand 2
        push    BP              ; save BP
        mov     BP,SP           ; get access to stack
        push    AX              ; push operand 1
        push    BX              ; . . .
        push    CX              ; . . .
        push    DX              ; . . .
        call    __fdiv_m64r     ; divide op 1 by op 2
        sub     sp,8            ; rtn popped parm, _ret87 wants it
        jmp     _ret87          ; goto common epilogue

__FDD87:
        fld     qword ptr ss:[SI] ; load operand 2
        push    BP              ; save BP
        mov     BP,SP           ; get access to stack
        push    AX              ; push operand 1
        push    BX              ; . . .
        push    CX              ; . . .
        push    DX              ; . . .
        fdivr   qword ptr -8[BP]; divide operand 1 by operand 2
        jmp     _ret87          ; goto common epilogue

__FDDemu:
        push    BP              ; save BP
        push    DI              ; save DI

;       f8split

        push    AX              ; push operand 1 onto stack
        push    BX              ; . . .
        push    CX              ; . . .
        push    DX              ; . . .
        mov     DI,SP           ; get address of operand 1
        _guess                  ; guess
          mov   BX,AX           ; - get most sig word of op1
          mov   AX,ss:6[SI]     ; - get most sig word of op2
          mov   DL,AL           ; - save mantissa-holding part
          mov   DH,BL           ; - save mantissa-holding part
          mov   cl,bh           ; - save sign
          and   BX,7FF0h        ; - isolate exponent
          _quif e               ; - quif zero
          mov   ch,ah           ; - save sign
          and   AX,7FF0h        ; - isolate exponent
          _quif e               ; - quif zero
          and   cx,8080h        ; - only want sign bits
          xor   dx,1010h        ; - set implied one bit of mant.
          xor   DL,AL           ; - . . .
          xor   DH,BL           ; - . . .
        _admit                  ; admit: one of the operands is 0
          add   SP,8            ; - clean up the stack
          pop   DI              ; - restore DI
          pop   BP              ; - restore BP
          or    ax,ax           ; - if the divisor is zero
          _if   e               ; - then
            mov ah,cl           ; - - set sign of inf
            jmp F8DivZero       ; - - return div by zero status
          _endif                ; - endif
          sub   AX,AX           ; - set result to 0
          sub   BX,BX           ; - . . .
          sub   CX,CX           ; - . . .
          sub   DX,DX           ; - . . .
          ret                   ; - return
        _endguess               ; endguess
        add     CH,CL           ; determine sign of quotient
        sub     BX,AX           ; calculate exponent of result
        add     bx,3ff0h        ; add in removed bias
        cmp     bx,0c000h       ; if expon >= $c000
        _if     ae              ; then FP underflow
          add   SP,8            ; - clean up the stack
          pop   DI              ; - restore DI
          pop   BP              ; - restore BP
          jmp   F8UnderFlow
        _endif                  ; endif
        cmp     bx,7ff0h        ; if $7ff0 >= exponent > $c000
        _if     ae              ; then FP overflow
          mov   AX,CX
          add   SP,8            ; - clean up the stack
          pop   DI              ; - restore DI
          pop   BP              ; - restore BP
          jmp   F8OverFlow
        _endif                  ; endif
;
;       special case if divisor is a power of 2 (added 19-may-88)
;
        mov     AX,ss:[SI]      ; get word of divisor into AX
        or      AX,ss:2[SI]     ; . . .
        _if     e               ; if fraction bits are zero
          mov   AX,ss:4[SI]     ; - get second word of divisor into AX
          test  AL,1Fh          ; - check bottom 5 bits of second word
        _endif                  ; endif
        _if     ne              ; if any of the bottom bits are non-zero
          jmp   long_div        ; - long division is required
        _endif                  ; endif
          _guess                ; - guess: divisor is power of 2
            or    AX,AX         ; - - quit if fraction bits not zero
            _quif ne            ; - - . . .
            cmp   DL,10h        ; - - quit if not just the implied 1 bit
            _quif ne            ; - - . . .
                                ; - - divisor is a power of 2
            and   CH,80h        ; - - isolate sign bit
            and   DH,0Fh        ; - - remove implied 1 bit
            mov   AX,BX         ; - - get exponent
            and   AH,7fh        ; - - get rid of sign bit
            or    AH,CH         ; - - put in correct sign
            or    AL,DH         ; - - get top 4 bits of mantissa

            mov   BX,ss:4[DI]   ; - - get rest of dividend
            mov   CX,ss:2[DI]   ; - - . . .
            mov   DX,ss:[DI]    ; - - . . .
            add   SP,8          ; - - clean up the stack
            pop   DI            ; - - restore DI
            pop   BP            ; - - restore BP
            ret                 ; - - return
          _endguess             ; - endguess

;         divisor contains only 1 bits in the most significant 16 bits
;         dividing DL into DH:4[DI]:2[DI]:0[DI]

          mov   BP,BX           ; - save exponent in BP
          push  CX              ; - save sign
          mov   BH,DL           ; - get divisor into BX
          xor   BL,BL           ; - . . .
          _shl  BX,1            ; - move to top of register
          _shl  BX,1            ; - . . .
          _shl  BX,1            ; - . . .
          mov   CL,5            ; - get shift count
          shr   AX,CL           ; - shift next 11 bits to bottom of AX
          or    BX,AX           ; - get all 16 bits together in BX (divisor)

          mov   DL,DH           ; - set up high order word of dividend
          xor   DH,DH           ; - . . .
          mov   AX,ss:4[DI]     ; - get next word of dividend
          div   BX              ; - do partial divide
          push  AX              ; - save next word of quotient
          mov   AX,ss:2[DI]     ; - get next word of dividend
          div   BX              ; - do partial divide
          push  AX              ; - save next word of quotient
          mov   AX,ss:[DI]      ; - get last word of dividend
          div   BX              ; - do partial divide
          push  AX              ; - save next word of quotient
          sub   AX,AX           ; - extend dividend and do 1 more divide
          div   BX              ; - . . .
          _shl  DX,1            ; - double remainder
          _if   nc              ; - if no carry
            cmp   DX,BX         ; - - compare against divisor
            cmc                 ; - - change the carry
          _endif                ; - endif
          rcr   DI,1            ; - get next bit of quotient from carry
          mov   DX,AX           ; - get last word of quotient
          pop   CX              ; - restore next word of quot
          pop   BX              ; - . . .
          pop   AX              ; - . . .
          add   BP,0010h        ; - adjust exponent
          test  AL,20h          ; - if answer needs aligning
          _if   e               ; - then
            sub   BP,0010h      ; - - adjust exponent back
            _loop               ; - - loop
              _shl  DI,1        ; - - - shift result left
              _rcl  DX,1        ; - - - . . .
              _rcl  CX,1        ; - - - . . .
              _rcl  BX,1        ; - - - . . .
              _rcl  AL,1        ; - - - . . .
              test  AL,20h      ; - - until msb is in bit 6
            _until ne           ; - - ...
          _endif                ; - endif
          and     AL,1Fh        ; - turn off implied one bit
          jmp     fin_up_div    ; - finish up divide
long_div:
        push    CX              ; push sign
        push    BX              ; push exponent

        mov     BP,3            ; initialize count
        push    BP              ; and save it on the stack

        mov     AL,DH           ; move high byte of dividend into AL
        mov     AH,DL           ; move high byte of divisor into AH
        mov     DX,ss:[DI]      ; move rest of dividend into registers
        mov     CX,ss:2[DI]     ; . . .
        mov     BX,ss:4[DI]     ; . . .

        mov     DI,1            ; initialize first quotient word

;<> This bit serves as a count, when it has been shifted left into the carry,
;<> we know it is time to get a fresh quotient word and save this one

        mov     BP,ss:4[SI]     ; get the next word of divisor into BP
        _loop                   ; loop
          _guess                ; - guess
            cmp  AH,AL          ; - - The purpose of this guess is to
            jb   try            ; - - determine if the divisor will subtract
            ja   didnt_go       ; - - from the dividend without a borrow, and to
            cmp  BP,BX          ; - - branch to the appropriate routine
            jb   try            ; - -
            ja   didnt_go       ; - -
            cmp  ss:2[SI],CX    ; - -
            _quif ne            ; - -
            cmp  ss:[SI],DX     ; - -
            je   try            ; - -
          _endguess             ; - endguess
          _if   c               ; - if the carry is set (ie the divisor will
                                ; - - definitely subtract from the divident
                                ; - - without a borrow
try:
            sub  DX,ss:[SI]     ; - - subtract divisor from dividend
            sbb  CX,ss:2[SI]    ; - - . . .
            sbb  BX,BP          ; - - . . .
            sbb  AL,AH          ; - - . . .
            stc                 ; - - set carry to indicate that divisor was
                                ; - - successfully subtracted from dividend
          _endif                ; - endif
didnt_go: _rcl  DI,1            ; - rotate 1 (if carry set) into quotient word
          jc    newquot         ; - if quotient word is full, get another
sh_div:   _shl  DX,1            ; - shift divident left
          _rcl  CX,1            ; - . . .
          _rcl  BX,1            ; - . . .
          _rcl  AL,1            ; - . . .

          cmp   AL,20h
          jae   try

;<> If bit 5 of dividend is set here, we didnt subtract the divisor from the
;<> dividend (recall that the divisor has a 1 in the msb -- if we subtracted
;<> it from the dividend without a borrow, the dividend would not have a one
;<> in its msb to be shifted into bit 5 tested for in the condition above. If
;<> we are rotating a bit into bit 5, the dividend is now big enough that we
;<> can be sure of subtracting out the divisor without a borrow, as we have
;<> shifted it left one digit.

          cmp   AL,10h
        _until  b

;<> If bit 4 of dividend is set and bit 5 is not set, we do not know if
;<> divisor will subtract from the dividend without a borrow (recall
;<> that the divisor always has its high bit -- corresponding to bit 4 --
;<> set to one (as it was normalized), so we jump to the compare code

          cmc
          jmp   didnt_go

;<> If bit 4 is clear, then we know that the divisor will not subtract
;<> from dividend without a borrow (see note above) so we just shift in
;<> a zero

;<><><> we need 55 bits of quotient; 52 will actually be saved, 1 will be
;<><><> implied, 1 will be used for rounding, and 1 is needed in case the
;<><><> most significant bit of the quotient turns out to be a zero.

newquot: pop    BP              ; - get loop count
        dec     BP              ; - decrement loop count
        js      enddiv          ; - exit if we have filled 55 bits of quotient
        push    DI              ; - save quotient word
        _if     e
          mov   DI,0200h
        _else
          mov   DI,1            ; - get a fresh quotient word
        _endif
        push    BP              ; - save loop count
        mov     BP,ss:4[SI]     ; - restore BP to hold part of divisor
        jmp     sh_div          ; - return to mainstream of code

enddiv: mov     DX,DI           ; put least sig bits in correct register
        xchg    DH,DL
        _shl    DH,1            ; and left justify them (there are seven)
        pop     CX              ; get rest of mantissa
        pop     BX              ; . . .
        pop     AX              ; . . .
        pop     BP              ; get exponent
        or      AX,AX           ; if high bit of AX is set
        _if     s               ; then
          add   BP,0010h        ; - increment exponent
          shr   AX,1            ; - shift result right by one
          rcr   BX,1            ; - . . .
          rcr   CX,1            ; - . . .
          rcr   DX,1            ; - . . .
        _endif                  ; endif
        and     AX,3fffh        ; turn off top bit
        shr     AX,1            ; shift result right by nine
        rcr     BX,1
        rcr     CX,1
        rcr     DX,1
        mov     DL,DH
        mov     DH,CL
        mov     CL,CH
        mov     CH,BL
        mov     BL,BH
        mov     BH,AL
        mov     AL,AH
        sub     AH,AH
fin_up_div:
        sub     BP,0010h        ; decrement exponent
        _if     be              ; if so
          pop   CX              ; - clear stack
          add   SP,8            ; - . . .
          pop   DI              ; - restore DI
          pop   BP              ; - restore bp
          jmp   F8UnderFlow     ; - report error
        _endif                  ; endif
;;;     jmp     fin_up          ; jump to generic finish up routine

;
; The following code consists of shared routines for the arithmetic ops
;
; fin_up is the exit point for the above routines in most cases
; fini    is also an exit point
; f8split is an 'initialization' routine to split the reals into parts
;

fin_up: add     DX,1            ; use last bit of mantissa
        adc     CX,0            ; to round it up
        adc     BX,0            ; . . .
        adc     AL,0            ; . . .
        test    AL,20h          ;
        _if     ne              ; if we carried, increment the exponent
          add   BP,0010h        ; - increment exponent
          cmp   BP,7ff0h        ; - if exponent too large
          _if   ae              ; - if overflow occurs
            pop   AX            ; - - restore sign
            add   SP,8          ; - - clean up stack
            pop   DI            ; - - restore DI
            pop   BP            ; - - restore BP
            jmp F8OverFlow      ; - - goto generic overflow handler
          _endif                ; - endif
          and   AX,001fh        ; - clear part of AX for the exponent/sign
                                ; - (for both div and mul)
        _endif                  ; endif
        shr     AX,1            ; shift mantissa to correct position
        rcr     BX,1            ; . . .
        rcr     CX,1            ; . . .
        rcr     DX,1            ; . . .
        or      AX,BP           ; or exponent into the real
        pop     BP              ; get sign
        and     BP,8000h        ; isolate it
        or      AX,BP           ; put it in real
        add     SP,8            ; clean up stack
        pop     DI              ; restore DI
        pop     BP              ; restore BP
        ret                     ; return to caller
        endproc __FDD

; F8split 'unnormalizes' two double precision reals.
; Operand 1 is passed in AX:BX:CX:DX
; Operand 2 is passed in SS:[SI]
; The reals are still in 'compressed' form when they arrive -- that is
; the implied 1 bit is hidden and the numbers are in excess 3FE.
; f8split moves the biased exponents of the reals into AX and BX.
; F8split turns on the implied '1' bit in each real,
; clears out the exponent from the mantissa words of each real, and
; returns the updated high-bits of the mantissa in DH/DL (op1/op2)
; The sign of op1 is in CL, sign of op2 in CH.
; If either real is found to be 0, we return at once with both reals
; unchanged from their initial states, and the zero flag is set. Note
; that the OR operation at the end of this routine reset the zero flag
; otherwise.


_chkadd: call   _chk8087
        go_to   fdadd

_chksub: call   _chk8087
        go_to   fdsub

_chkmul: call   _chk8087
        go_to   fdmul

_chkdiv: call   _chk8087
        go_to   fddiv

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
          mov   ax,offset __FDA87       ; - get addr of add rtn
          mov   fdadd,ax                ; - ...
          mov   ax,offset __FDS87       ; - get addr of sub rtn
          mov   fdsub,ax                ; - ...
          mov   ax,offset __FDM87       ; - get addr of mul rtn
          mov   fdmul,ax                ; - ...
          test  byte ptr __chipbug,1    ; - if we have a bad divider
          _if   ne                      ; - then
            mov ax,offset __FDDbad_div  ; - - get addr of div rtn
          _else                         ; - else
            mov ax,offset __FDD87       ; - - get addr of div rtn
          _endif
          mov   fddiv,ax                ; - ...
        _else                           ; else
          mov   ax,offset __FDAemu      ; - get addr of add rtn
          mov   fdadd,ax                ; - ...
          mov   ax,offset __FDSemu      ; - get addr of sub rtn
          mov   fdsub,ax                ; - ...
          mov   ax,offset __FDMemu      ; - get addr of mul rtn
          mov   fdmul,ax                ; - ...
          mov   ax,offset __FDDemu      ; - get addr of div rtn
          mov   fddiv,ax                ; - ...
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
