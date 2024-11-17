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


;========================================================================
;==     Name:           PIA,PIS                                        ==
;==     Operation:      Pointer integer add and subtract               ==
;==     Inputs:         DX:AX pointer                                  ==
;==                     CX:BX long int                                 ==
;==     Outputs:        DX:AX has DX:AX op CX:BX as  pointer           ==
;==     Volatile:       DX:AX and CX:BX                                ==
;==                                                                    ==
;========================================================================
include mdef.inc


extrn  "C",_HShift      : byte

        modstart        pia

        xdefp   __PIA
        xdefp   __PIS

        defp    __PIS
        neg     cx              ; negate the 32 bit integer
        neg     bx              ; ...
        sbb     cx,0            ; ...

        defp    __PIA
        add     ax,bx           ; add offsets
        adc     cx,0            ; calculate overflow
        mov     bx,cx           ; shuffle overflow info bx
if (_MODEL and (_BIG_DATA or _HUGE_DATA)) and ((_MODEL and _DS_PEGGED) eq 0)
        push    ds              ; need a segment register
        mov     cx,seg _HShift  ; get the huge shift value
        mov     ds,cx           ; ...
        mov     cl,ds:_HShift   ; ...
        pop     ds              ; restore register
else
        mov     cl,_HShift      ; get huge shift value
endif
        shl     bx,cl           ; adjust the overflow by huge shift value
        add     dx,bx           ; and add into selector value
        ret                     ; ...
        endproc __PIA
        endproc __PIS

        endmod
        end
