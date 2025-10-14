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
;=      Name:           PTS//PTC                                        =
;=      Operation:      Pointer subtract                                =
;=      Inputs:         DX:AX pointer                                   =
;=                      CX:BX pointer                                   =
;=      Outputs:        DX:AX has DX:AX - CX:BX as signed integer (PTS) =
;=                                DX:AX cmp CX:BX as an unsigned  (PTC) =
;=      Volatile:       DX:AX and CX:BX                                 =
;=                                                                      =
;========================================================================

compare_subtract        macro name

include mdef.inc

extrn   "C",_HShift     : byte

        modstart        &name

        xdefp   &name

        defp    &name

        push    si
        push    di
        push    cx

;
; this code changes the two pointers in dx:ax and cx:bx into representative
; integers in dx:ax and di:bx
;
; a representative integer for seg:off is:
;
; ((long) seg >> hshift ) + off
;
; the trick for getting the long shift to work without any loops is as follows:
;
; lo word = seg << ( 16 - hshift )
; hi word = seg >> hshift
;
; these shifts truncate useless info since they're logical shifts
;

        mov     si,dx
        mov     di,cx

if (_MODEL and (_BIG_DATA or _HUGE_DATA)) and ((_MODEL and _DS_PEGGED) eq 0)
        push    ds              ; save segment register
        mov     cx,seg _HShift  ; get huge shift value
        mov     ds,cx           ; ...
        mov     cl,ds:_HShift   ; prepare to shift
        pop     ds              ; restore segment register
else
        mov     cl,_HShift      ; get huge shift value
endif

        shr     dx,cl
        shr     di,cl

        neg     cl
        add     cl,16

        shl     si,cl

        add     ax,si
        adc     dx,0

        pop     si              ; get old cx value

        shl     si,cl

        add     bx,si
        adc     di,0

        do_stuff dx,ax,di,bx

        pop     di
        pop     si
        ret                      ; and return 32 bit integer in dx:ax

        endproc &name            ;

        endmod
        end
        endm
