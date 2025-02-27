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


; Name:         I4FS, U4FS
; Operation:    Convert Integer types to single precision
; Inputs:       DX;AX, unsigned or signed integer
; Outputs:      DX;AX single precision floating point (DX exp)
; Volatile:     none
;
include mdef.inc
include struct.inc

        modstart        i4fs086

        xdefp   __I4FS
        xdefp   __U4FS

        defpe   __I4FS
        or      dx,dx           ; check sign
        jns     __U4FS          ; if positive, just convert
        not     dx              ; take absolute value of number
        neg     ax
        sbb     dx,-1           ; (tricky negation)
        call    U4FS            ; convert to FS
        or      dh,080h         ; set sign bit on
        ret                     ; and return
        endproc __I4FS


        defpe   __U4FS
        call    U4FS            ; convert integer to float
        ret                     ; return
        endproc __U4FS


U4FS    proc    near
        or      dx,dx           ; check if high 16 bits zero
        jne     int32           ; do extra code if not
        or      ax,ax           ; if number is 0
        _if     e               ; then
          ret                   ; - return
        _endif                  ; endif
        mov     dx,7f00h+16*256 ; initialize exponent and dl
        _loop                   ; loop to normalize 16 bit mantissa
          dec     dh            ; - decrement exponent
          _shl    ax,1          ; - shift mantissa
        _until  c               ; until implied '1' bit shifted out
        mov     dl,ah           ; high bits of mantissa to dl
        mov     ah,al           ; low bits to ah
        xor     al,al           ; zero the rest
        shr     dx,1            ; move exponent & mantissa over
        rcr     ax,1            ; ...
        ret                     ; return

int32:  cmp     dh,0            ; first check if only 24 bits
        jne     full32          ; must do full 32 bits (could lose bits)
        mov     dh,7fh+24       ; initial exponent
        _loop                   ; loop to normalize 24 bit mantissa
          dec     dh            ; - decrement exponent
          _shl    ax,1          ; - shift mantissa
          _rcl    dl,1          ; - ...
        _until  c               ; until implied '1' bit shifted out
        shr     dx,1            ; shift exponent & mantissa
        rcr     ax,1            ; ...
        ret                     ; and return

                                ; 25 to 32 bit integer
full32: push    cx              ; save cx
        mov     cl,7fh+32       ; initial exponent
        _loop                   ; loop to normalize 32 bit mantissa
          dec     cl            ; - decrement exponent
          _shl    ax,1          ; - shift number over
          _rcl    dx,1          ; - ...
        _until  c               ; until implied '1' bit shifted out
        _shl    al,1            ; get next bit into carry
        mov     al,ah           ; move mantissa over 8 bits
        mov     ah,dl           ; . . .
        mov     dl,dh           ; . . .
        mov     dh,cl           ; get exponent

        adc     ax,0            ; round up if necessary
        adc     dx,0            ; (increment exponent if necessary)

        shr     dx,1            ; shift exponent & mantissa over
        rcr     ax,1            ; ...
        pop     cx              ; restore cx
        ret                     ; return
        endproc U4FS

        endmod
        end
