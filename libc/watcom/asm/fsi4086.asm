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
;==     Name:           FSI4, FSU4, FSI2, FSU2, FSI1, FSU1             ==
;==     Operation:      Convert single precision to integer            ==
;==     Inputs:         DX;AX   single precision floating point        ==
;==     Outputs:        DX;AX, AX, AL  integer value                   ==
;==     Volatile:       none                                           ==
;==                                                                    ==
;==                                                                    ==
;==                                     handle -1.0 -> 0xffffffff      ==
;========================================================================
include mdef.inc
include struct.inc

        modstart        fsi4086

        xdefp   __FSI4
        xdefp   __FSU4
        xdefp   __FSI2
        xdefp   __FSU2
        xdefp   __FSI1
        xdefp   __FSU1

        defpe   __FSI4
        push    cx              ; save cx
        mov     cl,7fh+31       ; maximum number 2^31-1
        call    __FSI           ; convert to integer
        pop     cx              ; restore cx
        ret                     ; return (overflow already handled
        endproc __FSI4

        defpe   __FSU4
        push    cx              ; save cx
        mov     cl,7fh+32       ; maximum number 2^32-1
        call    __FSU           ; convert to integer
        pop     cx              ; restore cx
        ret                     ; return if no overflow
        endproc __FSU4

        defpe   __FSI2
        push    cx              ; save cx
        mov     cl,7fh+15       ; maximum number 2^15-1
        call    __FSI           ; convert to integer
        pop     cx              ; restore cx
        ret                     ; return (overflow already handled
        endproc __FSI2

        defpe   __FSU2
        push    cx              ; save cx
        mov     cl,7fh+16       ; maximum number 2^16-1
        call    __FSU           ; convert to integer
        pop     cx              ; restore cx
        ret                     ; return if no overflow
        endproc __FSU2

        defpe   __FSI1
        push    cx              ; save cx
        mov     cl,7fh+7        ; maximum number 2^7-1
        call    __FSI           ; convert to integer
        pop     cx              ; restore cx
        ret                     ; return (overflow already handled
        endproc __FSI1


        defpe   __FSU1
        push    cx              ; save cx
        mov     cl,7fh+8        ; maximum number 2^8-1
        call    __FSU           ; convert to integer
        pop     cx              ; restore cx
        ret                     ; return if no overflow
        endproc __FSU1

__FSI   proc    near
__FSU:
        or      dx,dx           ; check sign bit
        jns     __FSAbs         ; treat as unsigned if positive
        call    __FSAbs         ; otherwise convert number
        not     dx              ; take two's complement of number
        neg     ax              ; ...
        sbb     dx,-1           ; ...
        ret                     ; and return
        endproc __FSI

; 18-nov-87 AFS (for WATCOM C)
;__FSU  proc near
;       jmp
;       or      dx,dx           ; check sign bit
;       jns     __FSAbs         ; just convert if positive
;       sub     ax,ax           ; return 0 if negative
;       sub     dx,dx           ; ...
;       ret
;       endproc __FSU

;========================================================================
;==     Name:           FSAbs_                                         ==
;==     Inputs:         DX;AX float                                    ==
;==                     CL    maximum exponent excess $7f              ==
;==     Outputs:        DX;AX integer, absolute value of float         ==
;==                           if exponent >= maximum then 2^max - 1    ==
;==                             returned                               ==
;========================================================================

__FSAbs proc near
        or      dx,dx           ; check if number 0
        je      retzero         ; if so, just return it
        _shl    ax,1            ; shift mantissa over
        _rcl    dx,1            ; and exponent as well
        cmp     dh,7fh          ; quit if number < 1.0          15-apr-91
        jb      uflow           ; ...
        stc                     ; set carry for implied bit
        rcr     dl,1            ; put implied '1' bit in
        rcr     ax,1            ; ...
        cmp     dh,cl           ; check if exponent exceeds maximum
        jae     retmax          ; return maximum value if so
        sub     dh,7fh+23       ; calculate amount to shift (+ve -> left)
        jae     m_left          ; jump if left shift/no shift
        _loop                   ; shift mantissa right
          shr     dl,1          ; - shift mantissa
          rcr     ax,1          ; - ...
          inc     dh            ; - increment shift count
        _until  e               ; until shift count is 0

;       rounding commented out by FWC 30-jan-87

;       adc     ax,0            ; round (nb 'inc' does not affect carry)
;       adc     dx,0            ; ripple (nb ah was 0)
        ret                     ; return with number

m_left:
        _if     ne              ; done if exponent exactly 23
          mov     cl,dh         ; - get shift count
          xor     dh,dh         ; - clear high byte
          _loop                 ; - loop to put mantissa in proper spot
            _shl  ax,1          ; - - multiply number by 2
            _rcl  dx,1          ; - - ...
            dec   cl            ; - - decrement shift count
          _until  e             ; - until done shift
        _endif                  ; endif
        ret                     ; return

retmax: mov     ax,0ffffh       ; return maximum value
        mov     dx,ax           ; ...
        _loop                   ; loop to shift maximum value over
          cmp     cl,7fh+32     ; - quit if exponent reached
          je      return        ; - ...
          shr     dx,1          ; - shift number over 1 bit
          rcr     ax,1          ; - ...
          inc     cl            ; - increment exponent value
        _endloop                ; endloop

uflow:  sub     ax,ax           ; set entire number to 0
retzero:sub     dx,dx           ; ensure entire number 0
return: ret                     ; return
        endproc __FSAbs

        endmod
        end
