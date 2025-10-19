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


;<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
;<>
;<> __FDC - floating double comparison
;<>     input:  AX:BX:CX:DX - operand 1
;<>             DS:SI - address of operand 2
;<>       if op1 > op2,  1 is returned in AX
;<>       if op1 < op2, -1 is returned in AX
;<>       if op1 = op2,  0 is returned in AX
;<>
;<>
;<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
include mdef.inc
include struct.inc

        modstart        fdc086

        xdefp   __FDC
        xdefp   __EDC

        defp    __EDC
        push    es:6[si]        ; push second operand onto stack
        push    es:4[si]        ; ...
        push    es:2[si]        ; ...
        push    es:[si]         ; ...
        mov     si,sp           ; point to second operand
        docall  __FDC           ; do the comparison
        lahf                    ; save flags
        add     sp,8            ; clean up stack
        or      ax,ax           ; clear overflow flag
        sahf                    ; restore flags
        cbw                     ; sign extend result
        ret                     ; return
        endproc __EDC

        defpe   __FDC
        push    BP              ; save BP
        push    DI              ; save DI
        test    AX,07ff0h       ; check for zero
        _if     e               ; if it is then
          sub   AX,AX           ; - make whole damn thing a zero
        _endif                  ; endif
        mov     DI,ss:6[SI]     ; get high word of arg2
        test    DI,07ff0h       ; check for zero
        _if     e               ; if it is then
          sub   DI,DI           ; - make whole damn thing a zero
        _endif                  ; endif
        mov     BP,DI           ; save op2 exponent
        xor     BP,AX           ; see about signs of the operands
        mov     BP,0            ; clear result
        js      cmpdone         ; quif arg1 & arg2 have diff signs
        _guess                  ; guess
          cmp   AX,DI           ; - compare high words of arg1, arg2
          _quif ne              ; - quif not equal
          cmp   BX,ss:4[SI]     ; - compare 2nd  words of arg1, arg2
          _quif ne              ; - quif not equal
          cmp   CX,ss:2[SI]     ; - compare 3rd  words of arg1, arg2
          _quif ne              ; - quif not equal
          cmp   DX,ss:[SI]      ; - compare 4th  words of arg1, arg2
        _endguess               ; endguess
        _if     ne              ; if arg1 <> arg2
          rcr   CX,1            ; - save carry in CX
          xor   AX,CX           ; - sign of BX is sign of result

cmpdone:  _shl  AX,1            ; - get sign of result into carry
          sbb   BP,0            ; - BP gets sign of result
          _shl  BP,1            ; - double BP
          inc   BP              ; - make BP -1 or 1
        _endif                  ; endif
        mov     AX,BP           ; get result
        pop     DI              ; restore DI
        pop     BP              ; restore BP
        ret                     ; return to caller
        endproc __FDC

        endmod
        end
