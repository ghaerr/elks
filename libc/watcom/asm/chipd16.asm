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
; static char sccs_id[] = "@(#)patch16.asm      1.8  12/21/94  14:53:49";
;
; This code is being published by Intel to users of the Pentium(tm)
; processor.  Recipients are authorized to copy, modify, compile, use and
; distribute the code.
;
; Intel makes no warranty of any kind with regard to this code, including
; but not limited to, implied warranties or merchantability and fitness for
; a particular purpose. Intel assumes no responsibility for any errors that
; may appear in this code.
;
; No patent licenses are granted, express or implied.
;
;
include mdef.inc

.386
.387
                name    fdiv_patch

_TEXT   SEGMENT PARA USE16 PUBLIC 'CODE'
fdiv_risc_table DB      0, 1, 0, 0, 4, 0, 0, 7, 0, 0, 10, 0, 0, 13, 0, 0
fdiv_scale_1    DD      03f700000h              ;0.9375
fdiv_scale_2    DD      03f880000h              ;1.0625
one_shl_63      DD      05f000000h


dispatch_table DW       offset label0
        DW      offset label1
        DW      offset label2
        DW      offset label3
        DW      offset label4
        DW      offset label5
        DW      offset label6
        DW      offset label7
        DW      offset label8
        DW      offset label9
        DW      offset label10
        DW      offset label11
        DW      offset label12
        DW      offset label13
        DW      offset label14
        DW      offset label15
        DW      offset label16
        DW      offset label17
        DW      offset label18
        DW      offset label19
        DW      offset label20
        DW      offset label21
        DW      offset label22
        DW      offset label23
        DW      offset label24
        DW      offset label25
        DW      offset label26
        DW      offset label27
        DW      offset label28
        DW      offset label29
        DW      offset label30
        DW      offset label31
        DW      offset label32
        DW      offset label33
        DW      offset label34
        DW      offset label35
        DW      offset label36
        DW      offset label37
        DW      offset label38
        DW      offset label39
        DW      offset label40
        DW      offset label41
        DW      offset label42
        DW      offset label43
        DW      offset label44
        DW      offset label45
        DW      offset label46
        DW      offset label47
        DW      offset label48
        DW      offset label49
        DW      offset label50
        DW      offset label51
        DW      offset label52
        DW      offset label53
        DW      offset label54
        DW      offset label55
        DW      offset label56
        DW      offset label57
        DW      offset label58
        DW      offset label59
        DW      offset label60
        DW      offset label61
        DW      offset label62
        DW      offset label63

;
;  Stack variables for divide routines.
;

DENOM           EQU     0
NUMER           EQU     12
PATCH_CW        EQU     28
PREV_CW         EQU     24
DENOM_SAVE      EQU     32
STACK_SIZE      EQU     44

MAIN_DENOM      EQU     0
MAIN_NUMER      EQU     12


SPILL_SIZE      EQU     12
if _MODEL and _BIG_CODE
MEM_OPERAND     EQU     10
else
MEM_OPERAND     EQU     8
endif
SPILL_MEM_OPERAND       EQU     (MEM_OPERAND+SPILL_SIZE)

;
;  Stack frame locations for MS C/C++
;

SINGLE1         EQU     6  + STACK_SIZE
SINGLE2         EQU     10 + STACK_SIZE

DOUBLE1         EQU     6  + STACK_SIZE
DOUBLE2         EQU     14 + STACK_SIZE

LDOUBLE1        EQU     6  + STACK_SIZE
LDOUBLE2        EQU     22 + STACK_SIZE

SRESULT         EQU     14
DRESULT         EQU     22
LDRESULT        EQU     38

ONESMASK        EQU     0e00h

SINGLE_NAN      EQU     07f80h
DOUBLE_NAN      EQU     07ff0h

ILLEGAL_OPC     EQU     6

f_stsw  macro   where
        fstsw   where
endm

fdivr_st        MACRO   reg_index, reg_index_minus1
        fstp    tbyte ptr [bp+DENOM]
IF      reg_index_minus1 GE 1
        fxch    st(reg_index_minus1)
ENDIF
        fstp    tbyte ptr [bp+NUMER]
        call    fdiv_main_routine
IF      reg_index_minus1 GE 1
        fxch    st(reg_index_minus1)
ENDIF
        fld     tbyte ptr [bp+NUMER]
        fxch    st(reg_index)
        add     sp, STACK_SIZE          ; update stack
ENDM

fdivr_sti       MACRO   reg_index, reg_index_minus1
        fstp    tbyte ptr [bp+NUMER]
IF      reg_index_minus1 GE 1
        fxch    st(reg_index_minus1)
ENDIF
        fstp    tbyte ptr [bp+DENOM]
        call    fdiv_main_routine
IF      reg_index_minus1 GE 1
        fxch    st(reg_index_minus1)
ENDIF
        fld     tbyte ptr [bp+NUMER]
        add     sp, STACK_SIZE          ; update stack
ENDM

fdivrp_sti      MACRO   reg_index, reg_index_minus1
        fstp    tbyte ptr [bp+NUMER]
IF      reg_index_minus1 GE 1
        fxch    st(reg_index_minus1)
ENDIF
        fstp    tbyte ptr [bp+DENOM]
        call    fdiv_main_routine
IF      reg_index_minus1 GE 1
        fxch    st(reg_index_minus1)
ENDIF
        add     sp, STACK_SIZE          ; update stack
ENDM

fdiv_st         MACRO   reg_index, reg_index_minus1
        fstp    tbyte ptr [bp+NUMER]
IF      reg_index_minus1 GE 1
        fxch    st(reg_index_minus1)
ENDIF
        fld     st
        fstp    tbyte ptr [bp+DENOM]
        fstp    tbyte ptr [bp+DENOM_SAVE]
        call    fdiv_main_routine
IF      reg_index_minus1 GE 1
        fxch    st(reg_index_minus1)
ENDIF
        fld     tbyte ptr [bp+DENOM_SAVE]
        fxch    st(reg_index)
        add     sp, STACK_SIZE          ; update stack
ENDM

fdiv_sti        MACRO   reg_index, reg_index_minus1
        fxch    st(reg_index)
        fstp    tbyte ptr [bp+NUMER]
IF      reg_index_minus1 GE 1
        fxch    st(reg_index_minus1)
ENDIF
        fld     st
        fstp    tbyte ptr [bp+DENOM]
        fstp    tbyte ptr [bp+DENOM_SAVE]
        call    fdiv_main_routine
IF      reg_index_minus1 GE 1
        fxch    st(reg_index_minus1)
ENDIF
        fld     tbyte ptr [bp+DENOM_SAVE]
        add     sp, STACK_SIZE          ; update stack
ENDM

fdivp_sti       MACRO   reg_index, reg_index_minus1
        fstp    tbyte ptr [bp+DENOM]
IF      reg_index_minus1 GE 1
        fxch    st(reg_index_minus1)
ENDIF
        fstp    tbyte ptr [bp+NUMER]
        call    fdiv_main_routine
IF      reg_index_minus1 GE 1
        fxch    st(reg_index_minus1)
ENDIF
        add     sp, STACK_SIZE          ; update stack
ENDM


        assume cs:_TEXT, ds:nothing, ss:nothing

                                        ; In this implementation the
                                        ; fdiv_main_routine is called,
                                        ; therefore all the stack frame
                                        ; locations are adjusted for the
                                        ; return pointer.

fdiv_main_routine PROC  NEAR

        fld     tbyte ptr [bp+MAIN_NUMER]    ; load the numerator
        fld     tbyte ptr [bp+MAIN_DENOM]       ; load the denominator
retry:

;  The following three lines test for denormals and zeros.
;  A denormal or zero has a 0 in the explicit digit to the left of the
;  binary point.  Since that bit is the high bit of the word, adding
;  it to itself will produce a carry if and only if the number is not
;  denormal or zero.
;
        mov     ax, [bp+MAIN_DENOM+6]   ; get mantissa bits 48-64
        add     ax,ax                   ; shift one's bit into carry
        jnc     denormal                ; if no carry, we have a denormal

;  The following three lines test the three bits after the four bit
;  pattern (1,4,7,a,d).  If these three bits are not all one, then
;  the denominator cannot expose the flaw.  This condition is tested by
;  inverting the bits and testing that all are equal to zero afterward.

        xor     ax, ONESMASK            ; invert the bits that must be one
        test    ax, ONESMASK            ; test for needed bits
        jz      scale_if_needed         ; if all one, it may need scalling
        fdivp   st(1), st               ; OK to use the hardware
        ret

;
;  Now we test the four bits for one of the five patterns.
;
scale_if_needed:
        shr     ax, 12                  ; keep first four bits after point
        push    bx                      ; save bx
        mov     bx,ax                   ; bx = index
        cmp     byte ptr fdiv_risc_table[bx], 0 ; check for (1,4,7,a,d)
        pop     bx                      ; restore bx
        jnz     divide_scaled
        fdivp   st(1), st               ; OK to use the hardware
        ret

divide_scaled:
        mov     ax, [bp + MAIN_DENOM+8] ; test denominator exponent
        and     ax, 07fffh              ; if pseudodenormal ensure that only
        jz      invalid_denom           ; invalid exception flag is set
        cmp     ax, 07fffh              ; if NaN or infinity  ensure that only
        je      invalid_denom           ; invalid exception flag is set
;
;  The following six lines turn off exceptions and set the
;  precision control to 80 bits.  The former is necessary to
;  force any traps to be taken at the divide instead of the scaling
;  code.  The latter is necessary in order to get full precision for
;  codes with incoming 32 and 64 bit precision settings.  If
;  it can be guaranteed that before reaching this point, the underflow
;  exception is masked and the precision control is at 80 bits, these
;  five lines can be omitted.
;
        fnstcw  [bp+PREV_CW]            ; save caller's control word
        mov     ax, [bp+PREV_CW]
        or      ax, 033fh               ; mask exceptions, pc=80
        and     ax, 0f3ffh              ; set rounding mode to nearest
        mov     [bp+PATCH_CW], ax
        fldcw   [bp+PATCH_CW]           ; mask exceptions & pc=80

;  The following lines check the numerator exponent before scaling.
;  This in order to prevent undeflow when scaling the numerator,
;  which will cause a denormal exception flag to be set when the
;  actual divide is preformed. This flag would not have been set
;  normally. If there is a risk of underflow, the scale factor is
;  17/16 instead of 15/16.
;
        mov     ax, [bp+MAIN_NUMER+8]   ; test numerator exponent
        and     ax, 07fffh
        cmp     ax, 00001h
        je      small_numer

; perform scaling of both numerator and denominator

        fmul    fdiv_scale_1            ; scale denominator by 15/16
        fxch
        fmul    fdiv_scale_1            ; scale numerator by 15/16
        fxch

;
;  The next line restores the users control word.  If the incoming
;  control word had the underflow exception masked and precision
;  control set to 80 bits, this line can be omitted.
;

        fldcw   [bp+PREV_CW]            ; restore caller's control word
        fdivp   st(1), st               ; OK to use the hardware
        ret

small_numer:
        fmul    fdiv_scale_2            ; scale denominator by 17/16
        fxch
        fmul    fdiv_scale_2            ; scale numerator by 17/16
        fxch

;
;  The next line restores the users control word.  If the incoming
;  control word had the underflow exception masked and precision
;  control set to 80 bits, this line can be omitted.
;

        fldcw   [bp+PREV_CW]            ; restore caller's control word
        fdivp   st(1), st               ; OK to use the hardware
        ret

denormal:
        mov     eax, [bp+MAIN_DENOM]            ; test for whole mantissa == 0
        or      eax, [bp+MAIN_DENOM+4]  ; test for whole mantissa == 0
        jnz     denormal_divide_scaled  ; denominator is not zero
invalid_denom:                          ; zero or invalid denominator
        fdivp   st(1), st               ; OK to use the hardware
        ret

denormal_divide_scaled:
        mov     ax, word ptr[bp + MAIN_DENOM + 8]       ; get exponent
        and     ax, 07fffh              ; check for zero exponent
        jnz     invalid_denom
;
;  The following six lines turn off exceptions and set the
;  precision control to 80 bits.  The former is necessary to
;  force any traps to be taken at the divide instead of the scaling
;  code.  The latter is necessary in order to get full precision for
;  codes with incoming 32 and 64 bit precision settings.  If
;  it can be guaranteed that before reaching this point, the underflow
;  exception is masked and the precision control is at 80 bits, these
;  six lines can be omitted.
;

        fnstcw  [bp+PREV_CW]            ; save caller's control word
        mov     ax, [bp+PREV_CW]
        or      ax, 033fh               ; mask exceptions, pc=80
        and     ax, 0f3ffh              ; set rounding mode to nearest
        mov     [bp+PATCH_CW], ax
        fldcw   [bp+PATCH_CW]           ; mask exceptions & pc=80

        mov     ax, [bp + MAIN_NUMER +8]        ; test numerator exponent
        and     ax, 07fffh              ; check for denormal numerator
        je      denormal_numer
        cmp     ax, 07fffh              ; NaN or infinity
        je      invalid_numer
        mov     ax, [bp + MAIN_NUMER + 6]       ; get bits 48..63 of mantissa
        add     ax, ax                  ; shift the first bit into carry
        jnc     invalid_numer           ; if there is no carry, we have an
                                        ; invalid numer
        jmp     numer_ok

denormal_numer:
        mov     ax, [bp + MAIN_NUMER + 6]       ; get bits 48..63 of mantissa
        add     ax, ax                  ; shift the first bit into carry
        jc      invalid_numer           ; if there is a carry, we have an
                                        ; invalid numer

numer_ok:
        fxch
        fstp    st                      ; pop numerator
        fld     st                      ; make copy of denominator
        fmul    [one_shl_63]            ; make denominator not denormal
        fstp    tbyte ptr [bp+MAIN_DENOM]
        fld     tbyte ptr [bp+MAIN_NUMER]       ; load numerator
        fxch                            ; restore proper order
        fwait

;  The next line restores the users control word.  If the incoming
;  control word had the underflow exception masked and precision
;  control set to 80 bits, this line can be omitted.
;
        fldcw   [bp+PREV_CW]            ; restore caller's control word
        jmp     retry                   ; start the whole thing over

invalid_numer:
;
;  The next line restores the users control word.  If the incoming
;  control word had the underflow exception masked and precision
;  control set to 80 bits, this line can be omitted.
;
        fldcw   [bp + PREV_CW]

        fdivp   st(1), st               ; use of hardware is OK.
        ret

fdiv_main_routine       ENDP

        public  __fdiv_fpr
        defpe   __fdiv_fpr

        push    bp
        push    bx
        sub     sp, STACK_SIZE
        shl     ax, 1
        mov     bx,ax
        mov     bp,sp
        jmp     word ptr dispatch_table[bx]


label0:
        fdiv    st,st(0)                ; D8 F0         FDIV    ST,ST(0)
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        ret
label1:
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        int     ILLEGAL_OPC
label2:
        fdivr   st,st(0)                ; D8 F8         FDIVR   ST,ST(0)
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        ret
label3:
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        int     ILLEGAL_OPC
label4:
        fdiv    st(0),st                ; DC F8/D8 F0   FDIV    ST(0),ST
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        ret
label5:
        fdivp   st(0),st                ; DE F8         FDIVP   ST(0),ST
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        ret
label6:
        fdivr   st(0),st                ; DC F0/DE F8   FDIVR   ST(0),ST
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        ret
label7:
        fdivrp  st(0),st                ; DE F0         FDIVRP  ST(0),ST
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        ret
label8:
        fdiv_st 1, 0
        pop     bx
        pop     bp
        ret
label9:
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        int     ILLEGAL_OPC
label10:
        fdivr_st 1, 0
        pop     bx
        pop     bp
        ret
label11:
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        int     ILLEGAL_OPC
label12:
        fdiv_sti 1, 0
        pop     bx
        pop     bp
        ret
label13:
        fdivp_sti 1, 0
        pop     bx
        pop     bp
        ret
label14:
        fdivr_sti 1, 0
        pop     bx
        pop     bp
        ret
label15:
        fdivrp_sti 1, 0
        pop     bx
        pop     bp
        ret
label16:
        fdiv_st 2, 1
        pop     bx
        pop     bp
        ret
label17:
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        int     ILLEGAL_OPC
label18:
        fdivr_st 2, 1
        pop     bx
        pop     bp
        ret
label19:
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        int     ILLEGAL_OPC
label20:
        fdiv_sti 2, 1
        pop     bx
        pop     bp
        ret
label21:
        fdivp_sti 2, 1
        pop     bx
        pop     bp
        ret
label22:
        fdivr_sti 2, 1
        pop     bx
        pop     bp
        ret
label23:
        fdivrp_sti 2, 1
        pop     bx
        pop     bp
        ret
label24:
        fdiv_st 3, 2
        pop     bx
        pop     bp
        ret
label25:
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        int     ILLEGAL_OPC
label26:
        fdivr_st 3, 2
        pop     bx
        pop     bp
        ret
label27:
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        int     ILLEGAL_OPC
label28:
        fdiv_sti 3, 2
        pop     bx
        pop     bp
        ret
label29:
        fdivp_sti 3, 2
        pop     bx
        pop     bp
        ret
label30:
        fdivr_sti 3, 2
        pop     bx
        pop     bp
        ret
label31:
        fdivrp_sti 3, 2
        pop     bx
        pop     bp
        ret
label32:
        fdiv_st 4, 3
        pop     bx
        pop     bp
        ret
label33:
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        int     ILLEGAL_OPC
label34:
        fdivr_st 4, 3
        pop     bx
        pop     bp
        ret
label35:
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        int     ILLEGAL_OPC
label36:
        fdiv_sti 4, 3
        pop     bx
        pop     bp
        ret
label37:
        fdivp_sti 4, 3
        pop     bx
        pop     bp
        ret
label38:
        fdivr_sti 4, 3
        pop     bx
        pop     bp
        ret
label39:
        fdivrp_sti 4, 3
        pop     bx
        pop     bp
        ret
label40:
        fdiv_st 5, 4
        pop     bx
        pop     bp
        ret
label41:
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        int     ILLEGAL_OPC
label42:
        fdivr_st 5, 4
        pop     bx
        pop     bp
        ret
label43:
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        int     ILLEGAL_OPC
label44:
        fdiv_sti 5, 4
        pop     bx
        pop     bp
        ret
label45:
        fdivp_sti 5, 4
        pop     bx
        pop     bp
        ret
label46:
        fdivr_sti 5, 4
        add     bx,bx
        pop     bx
        pop     bp
        ret
label47:
        fdivrp_sti 5, 4
        pop     bx
        pop     bp
        ret
label48:
        fdiv_st 6, 5
        pop     bx
        pop     bp
        ret
label49:
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        int     ILLEGAL_OPC
label50:
        fdivr_st 6, 5
        pop     bx
        pop     bp
        ret
label51:
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        int     ILLEGAL_OPC
label52:
        fdiv_sti 6, 5
        pop     bx
        pop     bp
        ret
label53:
        fdivp_sti 6, 5
        pop     bx
        pop     bp
        ret
label54:
        fdivr_sti 6, 5
        pop     bx
        pop     bp
        ret
label55:
        fdivrp_sti 6, 5
        pop     bx
        pop     bp
        ret
label56:
        fdiv_st 7, 6
        pop     bx
        pop     bp
        ret
label57:
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        int     ILLEGAL_OPC
label58:
        fdivr_st 7, 6
        pop     bx
        pop     bp
        ret
label59:
        add     sp, STACK_SIZE
        pop     bx
        pop     bp
        int     ILLEGAL_OPC
label60:
        fdiv_sti 7, 6
        pop     bx
        pop     bp
        ret
label61:
        fdivp_sti 7, 6
        pop     bx
        pop     bp
        ret
label62:
        fdivr_sti 7, 6
        pop     bx
        pop     bp
        ret
label63:
        fdivrp_sti 7, 6
        pop     bx
        pop     bp
        ret
__fdiv_fpr      ENDP


__fdivp_sti_st    PROC NEAR
                                ; for calling from mem routines
        sub     sp, STACK_SIZE
        mov     bp,sp
        fdivp_sti 1, 0
        ret
__fdivp_sti_st    ENDP

__fdivrp_sti_st   PROC NEAR
                                ; for calling from mem routines
        sub     sp, STACK_SIZE
        mov     bp,sp
        fdivrp_sti 1, 0
        ret
__fdivrp_sti_st   ENDP

public  __fdiv_chk
        defpe   __fdiv_chk
        push    bp
        sub     sp, STACK_SIZE
        mov     bp,sp
        fdivrp_sti 1, 0
        pop     bp
        ret
__fdiv_chk      ENDP

;;; FDIV_M32 - FDIV m32real FIX
;;
;;      Input : Value of the m32real in the top of STACK
;;
;;      Output: Result of FDIV in ST

        PUBLIC  __fdiv_m32
        defpe   __fdiv_m32

        push    eax                             ; save eax
        push    bp
        mov     bp,sp
        mov     ax, [bp + MEM_OPERAND + 2]      ; check for
        and     ax, SINGLE_NAN                  ; NaN
        cmp     ax, SINGLE_NAN                  ;
        je      memory_divide_m32               ;

        f_stsw  ax                              ; get status word
        and     ax, 3800h                       ; get top of stack
        je      spill_fpstack                   ; is FP stack full?
        fld     dword ptr[bp + MEM_OPERAND]     ; load m32real in ST
        call    __fdivp_sti_st                    ; do actual divide
        jmp     m32_ok                          ; return
spill_fpstack:
        fxch
        sub     sp, SPILL_SIZE          ; make temp space
        mov     bp,sp
        fstp    tbyte ptr[bp ]                  ; save user's ST(1)
        fld     dword ptr[bp + SPILL_MEM_OPERAND] ; load m32 real
        call    __fdivp_sti_st                    ; do actual divide
        fld     tbyte ptr[bp]                   ; restore user's ST(1)
                                                ;sp is adjusted by fdivp fn
        fxch
        add     sp, SPILL_SIZE
m32_ok:
        pop     bp
        pop     eax
        ret     4
memory_divide_m32:
        fdiv    dword ptr[bp + MEM_OPERAND]     ; do actual divide
        pop     bp
        pop     eax
        ret     4

__fdiv_m32        ENDP


;;; FDIV_M64 - FDIV m64real FIX
;;
;;      Input : Value of the m64real in the top of STACK
;;
;;      Output: Result of FDIV in ST

        PUBLIC  __fdiv_m64
        defpe   __fdiv_m64

        push    eax                             ; save eax
        push    bp
        mov     bp,sp
        mov     ax, [bp + MEM_OPERAND + 6]      ; check for
        and     ax, SINGLE_NAN                  ; NaN
        cmp     ax, SINGLE_NAN                  ;
        je      memory_divide_m64               ;

        f_stsw  ax                              ; get status word
        and     ax, 3800h                       ; get top of stack
        je      spill_fpstack_m64               ; is FP stack full?
        fld     qword ptr[bp + MEM_OPERAND]     ; load m64real in ST
        call    __fdivp_sti_st                    ; do actual divide
        jmp     m64_ok                          ; return
spill_fpstack_m64:
        fxch
        sub     sp, SPILL_SIZE          ; make temp space
        mov     bp,sp
        fstp    tbyte ptr[bp]                   ; save user's ST(1)
        fld     qword ptr[bp + SPILL_MEM_OPERAND] ; load m64real
        call    __fdivp_sti_st                    ; do actual divide
        fld     tbyte ptr[bp]                   ; restore user's ST(1)
                                                ;sp is adjusted by fdivp fn
        fxch
        add     sp, SPILL_SIZE
m64_ok:
        pop     bp
        pop     eax
        ret     8
memory_divide_m64:
        fdiv    qword ptr[bp + MEM_OPERAND]     ; do actual divide
        pop     bp
        pop     eax
        ret     8

__fdiv_m64        ENDP



;;; FDIVR_M32 - FDIVR m32real FIX
;;
;;      Input : Value of the m32real in the top of STACK
;;
;;      Output: Result of FDIVR in ST

        PUBLIC  __fdiv_m32r
        defpe   __fdiv_m32r
        push    eax                             ; save eax
        push    bp
        mov     bp,sp
        mov     ax, [bp + MEM_OPERAND + 2]      ; check for
        and     ax, SINGLE_NAN                  ; NaN
        cmp     ax, SINGLE_NAN                  ;
        je      memory_divide_m32r              ;

        f_stsw  ax                              ; get status word
        and     ax, 3800h                       ; get top of stack
        je      spill_fpstack_m32r              ; is FP stack full?
        fld     dword ptr[bp + MEM_OPERAND]     ; load m32real in ST
        call    __fdivrp_sti_st                   ; do actual divide
        jmp     m32r_ok                         ; return
spill_fpstack_m32r:
        fxch
        sub     sp, SPILL_SIZE          ; make temp space
        mov     bp,sp
        fstp    tbyte ptr[bp ]                  ; save user's ST(1)
        fld     dword ptr[bp + SPILL_MEM_OPERAND] ; load m32 real
        call    __fdivrp_sti_st                   ; do actual divide
        fld     tbyte ptr[bp]                   ; restore user's ST(1)
                                                ;sp is adjusted by fdivrp fn
        fxch
        add     sp, SPILL_SIZE
m32r_ok:
        pop     bp
        pop     eax
        ret     4
memory_divide_m32r:
        fdivr   dword ptr[bp + MEM_OPERAND]     ; do actual divide
        pop     bp
        pop     eax
        ret     4

__fdiv_m32r       ENDP


;;; FDIVR_M64 - FDIVR m64real FIX
;;
;;      Input : Value of the m64real in the top of STACK
;;
;;      Output: Result of FDIVR in ST

        PUBLIC  __fdiv_m64r
        defpe   __fdiv_m64r
        push    eax                             ; save eax
        push    bp
        mov     bp,sp
        mov     ax, [bp + MEM_OPERAND + 6]      ; check for
        and     ax, SINGLE_NAN                  ; NaN
        cmp     ax, SINGLE_NAN                  ;
        je      memory_divide_m64r              ;

        f_stsw  ax                              ; get status word
        and     ax, 3800h                       ; get top of stack
        je      spill_fpstack_m64r              ; is FP stack full?
        fld     qword ptr[bp + MEM_OPERAND]     ; load m64real in ST
        call    __fdivrp_sti_st                   ; do actual divide
        jmp     m64r_ok                         ; return
spill_fpstack_m64r:
        fxch
        sub     sp, SPILL_SIZE          ; make temp space
        mov     bp,sp
        fstp    tbyte ptr[bp ]                  ; save user's ST(1)
        fld     qword ptr[bp + SPILL_MEM_OPERAND] ; load m64real
        call    __fdivrp_sti_st                   ; do actual divide
        fld     tbyte ptr[bp]                   ; restore user's ST(1)
                                                ;sp is adjusted by fdivrp fn
        fxch
        add     sp, SPILL_SIZE
m64r_ok:
        pop     bp
        pop     eax
        ret     8
memory_divide_m64r:
        fdivr   qword ptr[bp + MEM_OPERAND]     ; do actual divide
        pop     bp
        pop     eax
        ret     8


__fdiv_m64r       ENDP


comment ~****************************************************************

;;; FDIV_M16I - FDIV m16int FIX
;;
;;      Input : Value of the m16int in the top of STACK
;;
;;      Output: Result of FDIV in ST

        PUBLIC  FDIV_M16I
FDIV_M16I       PROC    FAR
        push    eax                             ; save eax
        push    bp
        f_stsw  ax                              ; get status word
        and     ax, 3800h                       ; get top of stack
        je      spill_fpstack_m16i              ; is FP stack full?
        mov     bp,sp
        fild    word ptr[bp + MEM_OPERAND]      ; load m16int in ST
        call    fdivp_sti_st                    ; do actual divide
        jmp     m16i_ok                         ; return
spill_fpstack_m16i:
        fxch
        sub     sp, SPILL_SIZE          ; make temp space
        mov     bp,sp
        fstp    tbyte ptr[bp ]                  ; save user's ST(1)
        fild    word ptr[bp + SPILL_MEM_OPERAND] ; load m16int
        call    fdivp_sti_st                    ; do actual divide
        fld     tbyte ptr[bp]                   ; restore user's ST(1)
                                                ;sp is adjusted by fdivp fn
        fxch
        add     sp, SPILL_SIZE
m16i_ok:
        pop     bp
        pop     eax
        ret

FDIV_M16I       ENDP

;;; FDIV_M32I - FDIV m16int FIX
;;
;;      Input : Value of the m16int in the top of STACK
;;
;;      Output: Result of FDIV in ST

        PUBLIC  FDIV_M32I
FDIV_M32I       PROC    FAR
        push    eax                             ; save eax
        push    bp
        f_stsw  ax                              ; get status word
        and     ax, 3800h                       ; get top of stack
        je      spill_fpstack_m32i              ; is FP stack full?
        mov     bp,sp
        fild    dword ptr[bp + MEM_OPERAND]     ; load m32int in ST
        call    fdivp_sti_st                    ; do actual divide
        jmp     m32i_ok                         ; return
spill_fpstack_m32i:
        fxch
        sub     sp, SPILL_SIZE          ; make temp space
        mov     bp,sp
        fstp    tbyte ptr[bp ]                  ; save user's ST(1)
        fild    dword ptr[bp + SPILL_MEM_OPERAND] ; load m32int
        call    fdivp_sti_st                    ; do actual divide
        fld     tbyte ptr[bp]                   ; restore user's ST(1)
                                                ;sp is adjusted by fdivp fn
        fxch
        add     sp, SPILL_SIZE
m32i_ok:
        pop     bp
        pop     eax
        ret


FDIV_M32I       ENDP


;;; FDIVR_M16I - FDIVR m16int FIX
;;
;;      Input : Value of the m16int in the top of STACK
;;
;;      Output: Result of FDIVR in ST

        PUBLIC  FDIVR_M16I
FDIVR_M16I      PROC    FAR
        push    eax                             ; save eax
        push    bp
        f_stsw  ax                              ; get status word
        and     ax, 3800h                       ; get top of stack
        je      spill_fpstack_m16ir             ; is FP stack full?
        mov     bp,sp
        fild    word ptr[bp + MEM_OPERAND]      ; load m16int in ST
        call    fdivrp_sti_st                   ; do actual divide
        jmp     m16ir_ok                                ; return
spill_fpstack_m16ir:
        fxch
        sub     sp, SPILL_SIZE          ; make temp space
        mov     bp,sp
        fstp    tbyte ptr[bp ]                  ; save user's ST(1)
        fild    word ptr[bp + SPILL_MEM_OPERAND] ; load m16int
        call    fdivrp_sti_st                   ; do actual divide
        fld     tbyte ptr[bp]                   ; restore user's ST(1)
                                                ;sp is adjusted by fdivrp fn
        fxch
        add     sp, SPILL_SIZE
m16ir_ok:
        pop     bp
        pop     eax
        ret


FDIVR_M16I      ENDP


;;; FDIVR_M32I - FDIVR m32int FIX
;;
;;      Input : Value of the m32int in the top of STACK
;;
;;      Output: Result of FDIVR in ST

        PUBLIC  FDIVR_M32I
FDIVR_M32I      PROC    FAR
        push    eax                             ; save eax
        push    bp
        f_stsw  ax                              ; get status word
        and     ax, 3800h                       ; get top of stack
        je      spill_fpstack_m32ir             ; is FP stack full?
        mov     bp,sp
        fild    dword ptr[bp + MEM_OPERAND]     ; load m32int in ST
        call    fdivrp_sti_st                   ; do actual divide
        jmp     m32ir_ok                                ; return
spill_fpstack_m32ir:
        fxch
        sub     sp, SPILL_SIZE          ; make temp space
        mov     bp,sp
        fstp    tbyte ptr[bp ]                  ; save user's ST(1)
        fild    dword ptr[bp + SPILL_MEM_OPERAND] ; load m32int
        call    fdivrp_sti_st                   ; do actual divide
        fld     tbyte ptr[bp]                   ; restore user's ST(1)
                                                ;sp is adjusted by fdivrp fn
        fxch
        add     sp, SPILL_SIZE
m32ir_ok:
        pop     bp
        pop     eax
        ret

FDIVR_M32I      ENDP

************************************************************************~


_TEXT   ENDS

        end
