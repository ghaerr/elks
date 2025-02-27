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
;* Description:  Floating-point exception signaling
;*
;*****************************************************************************


include mdef.inc
include struct.inc
include fstatus.inc

        modstart        fstat386

        xrefp   "C",__set_ERANGE
        xrefp   __FPE_exception_

        datasegment
        enddata

        assume  ss:nothing

        xdefp   FPUnderFlow
        xdefp   FPInvalidOp
        xdefp   FPDivZero
        xdefp   FPOverFlow
        xdefp   F8UnderFlow
;        xdefp   F8InvalidOp
        xdefp   F8DivZero
        xdefp   F8OverFlow
        xdefp   F8RetInf
        xdefp   F4UnderFlow
;        xdefp   F4InvalidOp
        xdefp   F4DivZero
        xdefp   F4OverFlow
        xdefp   F4RetInf

;
;       FPUnderFlow( void ) : void
;
        defpe   FPUnderFlow
        push    eax                 ; save EAX
;;      mov     eax,FPE_UNDERFLOW   ; indicate underflow
;;      call    __FPE_exception_    ;
        pop     eax                 ; restore EAX
        ret                         ; return
        endproc FPUnderFlow

;
;       FPInvalidOp( void ) : void
;
        defpe   FPInvalidOp
        push    eax                 ; save EAX
        mov     eax,FPE_ZERODIVIDE  ; indicate divide by 0
        call    __FPE_exception_    ;
        pop     eax                 ; restore EAX
        ret                         ; return
        endproc FPInvalidOp

;
;       FPDivZero( void ) : void
;
        defpe   FPDivZero
        push    eax                 ; save EAX
        mov     eax,FPE_ZERODIVIDE  ; indicate divide by 0
        call    __FPE_exception_    ;
        pop     eax                 ; restore EAX
        ret                         ; return
        endproc FPDivZero

;
;       FPOverFlow( void ) : void
;
        defpe   FPOverFlow
        push    eax                 ; save EAX
        call    __set_ERANGE        ; errno = ERANGE
        mov     eax,FPE_OVERFLOW    ; indicate overflow
        call    __FPE_exception_    ;
        pop     eax                 ; restore EAX
        ret                         ; return
        endproc FPOverFlow

;
;       F8UnderFlow( void ) : reallong
;
        defp    F8UnderFlow
        xor     edx,edx             ; return zero
;
;       F4UnderFlow( void ) : real
;
        defp    F4UnderFlow
        call    FPUnderFlow         ; handle underflow
        xor     eax,eax             ; return zero
        ret                         ; return
        endproc F4UnderFlow
        endproc F8UnderFlow

;
;       F4DivZero( sign : int ) : real
;
        defp    F4DivZero
        call    FPDivZero           ; handle divide by 0
        jmp short F4RetInf          ; return Infinity
;
;       F4OverFlow( sign : int ) : real
;
        defp    F4OverFlow
        call    FPOverFlow          ; handle overflow
;
;       F4RetInf( sign : int ) : real
;
        defp    F4RetInf
        and     eax,80000000h       ; get sign
        or      eax,7F800000h       ; set infinity
        ret                         ; return
        endproc F4RetInf
        endproc F4OverFlow
        endproc F4DivZero

;
;       F8DivZero( sign : int ) : reallong
;
        defp    F8DivZero
        call    FPDivZero           ; handle divide by 0
        jmp short F8RetInf          ; return Infinity
;
;       F8OverFlow( sign : int ) : reallong
;
        defp    F8OverFlow
        call    FPOverFlow          ; handle overflow
;
;       F8RetInf( sign : int ) : reallong
;
        defp    F8RetInf
        and     eax,80000000h       ; get sign
        or      eax,7FF00000h       ; set infinity
        mov     edx,eax             ;
        sub     eax,eax             ; ...
        ret                         ; return
        endproc F8RetInf
        endproc F8OverFlow
        endproc F8DivZero

        endmod
        end
