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


;=========================================================================
;==     Name:           FDFS                                            ==
;==     Operation:      Float double to float single conversion         ==
;==     Inputs:         AX;BX;CX;DX     double precision float          ==
;==     Outputs:        DX;AX           single precision float          ==
;==     Volatile:       CX, DX destroyed                                ==
;=========================================================================
include mdef.inc
include struct.inc

        modstart        fdfs086

        xdefp   __FDFS

        defpe   __FDFS
        test    ax,07ff0h       ; check exponent
        je      retzero         ; if exponent = 0 then just return 0
        mov     dh,ah           ; save sign bit in dh
        and     dh,80h          ; clean rest of dh
        _shl    cx,1            ; shift number over
        _rcl    bx,1            ; ...
        _rcl    ax,1            ; shift out sign bit
        add     ch,20h          ; round floating point number
        adc     bx,0            ; ...
        adc     ax,0            ; increment exponent if need be
        je      oflow           ; overflow if exponent went to 0
        cmp     ax,2*16*(03ffh+80h) ; check for maximum exponent
        jae     oflow           ; overflow if above or equal
        cmp     ax,2*16*(03ffh-7eh) ; check for minimum exponent
        jb      uflow           ; underflow if below
        sub     ax,2*16*(03ffh-7fh) ; correct bias
        _shl    cx,1            ; do rest of shift
        _rcl    bx,1            ; ...
        _rcl    ax,1            ; ...
        _shl    cx,1            ; ...
        _rcl    bx,1            ; ...
        _rcl    ax,1            ; ...
        or      ah,dh           ; put in sign bit
        mov     dx,bx           ; put low part of mantissa in correct reg
        xchg    ax,dx           ; flip results  FWC 86-09-24
        ret                     ; return

oflow:  mov     ax,7f80h        ; return maximum possible number
        or      ah,dh           ; put in sign bit
        sub     dx,dx           ; ...
        xchg    ax,dx           ; flip results  FWC 86-09-24
        ret                     ; and return

uflow:
retzero:sub     ax,ax
        mov     dx,ax           ; return ax;dx = 0
        ret
        endproc __FDFS

        endmod
        end
