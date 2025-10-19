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
;* Modified by ghaerr for 8086 CPU (16-bits)
;*  32-bit "float"  type stored in DX:AX
;*  64-bit "double" type stored in AX:BX:CX:DX
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
        push    ax                  ; save EAX
;;      mov     ax,FPE_UNDERFLOW    ; indicate underflow
;;      call    __FPE_exception_    ;
        pop     ax                  ; restore EAX
        ret                         ; return
        endproc FPUnderFlow

;
;       FPInvalidOp( void ) : void
;
        defpe   FPInvalidOp
        push    ax                  ; save EAX
        mov     ax,FPE_ZERODIVIDE   ; indicate divide by 0
        call    __FPE_exception_    ;
        pop     ax                  ; restore EAX
        ret                         ; return
        endproc FPInvalidOp

;
;       FPDivZero( void ) : void
;
        defpe   FPDivZero
        push    ax                  ; save EAX
        mov     ax,FPE_ZERODIVIDE   ; indicate divide by 0
        call    __FPE_exception_    ;
        pop     ax                  ; restore EAX
        ret                         ; return
        endproc FPDivZero

;
;       FPOverFlow( void ) : void
;
        defpe   FPOverFlow
        push    ax                  ; save EAX
        call    __set_ERANGE        ; errno = ERANGE
        mov     ax,FPE_OVERFLOW     ; indicate overflow
        call    __FPE_exception_    ;
        pop     ax                  ; restore EAX
        ret                         ; return
        endproc FPOverFlow

;
;       F8UnderFlow( void ) : reallong
;
        defp    F8UnderFlow
        xor     bx,bx               ; return zero
        xor     cx,cx
;
;       F4UnderFlow( void ) : real
;
        defp    F4UnderFlow
        call    FPUnderFlow         ; handle underflow
        xor     dx,dx               ; return zero
        xor     ax,ax
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
        and     dx,8000h            ; get sign
        or      dx,7F80h            ; set infinity
        xor     ax,ax
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
        and     ax,8000h            ; get sign
        or      ax,7FF0h            ; set infinity
        xor     bx,bx
        xor     cx,cx
        xor     dx,dx
        ret                         ; return
        endproc F8RetInf
        endproc F8OverFlow
        endproc F8DivZero

        endmod
        end
