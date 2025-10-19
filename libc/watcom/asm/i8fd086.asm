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
; __I8FD converts signed 64-bit integer into a double
; __U8FD converts unsigned 64-bit integer into a double
;
; Input:    AX BX CX DX = 64-bit signed/unsigned integer
; Output:   AX BX CX DX = double
; Volatile: None
;
; 24-jan-2000   SJHowe          initial implementation

include mdef.inc
include struct.inc

        modstart        i8fd086

        xrefp   __U4FD

        xdefp   __I8FD
        defp    __I8FD
        or      ax,ax           ; test top word for being 0, +ve or -ve
        je      chkuint48       ; 0 => check next word down within __U8FD
        jg      uint64          ; +ve => convert as unsigned __int64
                                ; -ve => need to negate before calling __U8FD

                                ; negate 64-bit integer

; rely on the crucial fact that the Intel instruction "not" does not alter
; any flags. Borrows from previous register needs to propagated in the
; direction of the most significant word.

        neg     dx              ; negate
        not     cx              ; :
        sbb     cx,0FFFFH       ; :
        not     bx              ; :
        sbb     bx,0FFFFH       ; :
        not     ax              ; :
        sbb     ax,0FFFFH       ; :
        lcall   __U8FD          ; convert as unsigned 64-bit
        or      ah,80h          ; set sign bit
        ret                     ; return
        endproc __I8FD

; Convert unsigned 64-bit integer to double for x86
; Input: [AX BX CX DX] = 64-bit integer
; Output: [AX BX CX DX] = 64-bit double

        xdefp   __U8FD
        defp    __U8FD
        or      ax,ax           ; check if 0
        jne     uint64          ; no, do 64-bit conversion
chkuint48:
        or      bx,bx           ; check if 0
        jne     uint48          ; no, do 48-bit conversion

; high dword is 0, therefore use __U4FD for conversion
        mov     ax,dx           ; prepare registers for __U4FD
        mov     dx,cx           ; :
        jmp     __U4FD          ; call unsigned long => double cast function

; up to 48-bits set, rounding due to 54th bit cannot occur, therefore
; adjust exponent and do the easy conversion
uint48:
        push    si              ; save register
        mov     si,3FF0h+(36*16); create double's exponent
        mov     ax,bx           ; shift registers left 16 bits ...
        mov     bx,cx           ; :
        mov     cx,dx           ; :
        sub     dx,dx           ; :
        jmp     uint64easy      ; ... and continue as uint64 easy case

uint64:
        push    si              ; save register
        mov     si,3FF0h+(52*16); create double's exponent
        test    dx,07FFh        ; see if last bits of mantissa set ?
        je      uint64easy      ; no, do not worry about 54th bit

; the hard case, have to worry about rounding from 54th bit in mantissa

; a small optimisation, it is only the final right shift where the 54th bit
; matters, therefore telescope right shifting 1-11 times down to 1-8,1-3

        test    ah,0E0h         ; top 3 bits set?
        je      uint64hard1     ; no, jump
        mov     dl,dh           ; move registers right 8 bits ...
        mov     dh,cl           ; :
        mov     cl,ch           ; :
        mov     ch,bl           ; :
        mov     bl,bh           ; :
        mov     bh,al           ; :
        mov     al,ah           ; :
        sub     ah,ah           ; :
        add     si,0080h        ; adjust exponent
        jmp     uintshiftr      ; now shift just these 3 bits

uint64hard1:
        test    ax,0FFE0h       ; shift bits right ?
        je      uint64easy      ; no, treat as easy case
uintshiftr:
        push    di              ; save register
uintshiftr1:
        shr     ax,1            ; shifting right by 1 bit
        rcr     bx,1            ; :
        rcr     cx,1            ; :
        rcr     dx,1            ; :
        rcr     di,1            ; :
        add     si,0010h        ; adjust exponent
        test    ax,0FFE0h       ; implied 1 in place ?
        jne     uintshiftr1     ; no, loop again
        and     al,0Fh          ; get rid of implied 1
        or      ax,si           ; merge in exponent
        shl     di,1            ; move 54th sig bit into carry
        adc     dx,0            ; round up as necessary
        adc     cx,0            ; :
        adc     bx,0            ; :
        adc     ax,0            ; :
        pop     di              ; restore registers
        pop     si              ; :
        ret                     ; return

; shift without worry, no rounding problems
uint64easy:
        or      ah,ah           ; top word 0 ?
        _if ne
           mov  dl,dh           ; move registers right 8 bits ...
           mov  dh,cl           ; :
           mov  cl,ch           ; :
           mov  ch,bl           ; :
           mov  bl,bh           ; :
           mov  bh,al           ; :
           mov  al,ah           ; :
           sub  ah,ah           ; :
           add  si,0080h        ; adjust exponent
        _endif

; at this point, al is non-zero, see if shift right up to 3 times or left up
; to 4 times, or none. The sequence is borrowed from __U4FD

        test    al,0E0h         ; leading 1 to right of implied 1 ?
        _if ne                  ; yes, shift right up to 3 times
          shr   ax,1
          rcr   bx,1
          rcr   cx,1
          rcr   dx,1
          add   si,0010h        ; exponent++
          test  al,0E0h         ; leading 1 in implied 1 position ?
          je    uint64quit      ; must be, quit
          shr   ax,1
          rcr   bx,1
          rcr   cx,1
          rcr   dx,1
          add   si,0010h        ; exponent++
          test  al,0E0h         ; leading 1 in implied 1 position ?
          je    uint64quit      ; must be, quit
          shr   ax,1
          rcr   bx,1
          rcr   cx,1
          rcr   dx,1
          add   si,0010h        ; exponent++
          jmp   uint64quit      ; leading 1 must be in implied 1 position, quit
        _else
          test  al,10h          ; leading 1 in implied 1 position ?
          jne   uint64quit      ; yes, quit
          _shl  dx,1            ; shift mantissa left by 1 bit
          _rcl  cx,1            ; :
          _rcl  bx,1            ; :
          _rcl  ax,1            ; :
          sub   si,0010h        ; exponent--
          test  al,10h          ; leading 1 in implied 1 position ?
          jne   uint64quit      ; yes, quit
          _shl  dx,1            ; shift mantissa left by 1 bit
          _rcl  cx,1            ; :
          _rcl  bx,1            ; :
          _rcl  ax,1            ; :
          sub   si,0010h        ; exponent--
          test  al,10h          ; leading 1 in implied 1 position ?
          jne   uint64quit      ; yes, quit
          _shl  dx,1            ; shift mantissa left by 1 bit
          _rcl  cx,1            ; :
          _rcl  bx,1            ; :
          _rcl  ax,1            ; :
          sub   si,0010h        ; exponent--
          test  al,10h          ; leading 1 in implied 1 position ?
          jne   uint64quit      ; yes, quit
          _shl  dx,1            ; shift mantissa left by 1 bit
          _rcl  cx,1            ; :
          _rcl  bx,1            ; :
          _rcl  ax,1            ; :
          sub   si,0010h        ; exponent--
        _endif

uint64quit:
        and     al,0Fh          ; get rid of implied 1
        or      ax,si           ; merge in exponent
        pop     si              ; restore register
        ret                     ; return
        endproc __U8FD

        endmod
        end

