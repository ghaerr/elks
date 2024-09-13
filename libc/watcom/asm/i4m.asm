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
;==     Name:           I4M,U4M                                        ==
;==     Operation:      integer four byte multiply                     ==
;==     Inputs:         DX;AX   integer M1                             ==
;==                     CX;BX   integer M2                             ==
;==     Outputs:        DX;AX   product                                ==
;==     Volatile:       CX, BX destroyed                               ==
;========================================================================
include mdef.inc
include struct.inc

        modstart        i4m

        xdefp   __I4M
        xdefp   __U4M

        defp    __I4M
        defp    __U4M
        xchg    ax,bx           ; swap low(M1) and low(M2)
        push    ax              ; save low(M2)
        xchg    ax,dx           ; exchange low(M2) and high(M1)
        or      ax,ax           ; if high(M1) non-zero
        _if     ne              ; then
          mul   dx              ; - low(M2) * high(M1)
        _endif                  ; endif
        xchg    ax,cx           ; save that in cx, get high(M2)
        or      ax,ax           ; if high(M2) non-zero
        _if     ne              ; then
          mul   bx              ; - high(M2) * low(M1)
          add   cx,ax           ; - add to total
        _endif                  ; endif
        pop     ax              ; restore low(M2)
        mul     bx              ; low(M2) * low(M1)
        add     dx,cx           ; add previously computed high part
        ret                     ; and return!!!
        endproc __U4M
        endproc __I4M

        endmod
        end
