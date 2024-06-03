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


include mdef.inc
include struct.inc

;========================================================================
;==     Name:           I4D                                            ==
;==     Operation:      Signed 4 byte divide                           ==
;==     Inputs:         DX;AX   Dividend                               ==
;==                     CX;BX   Divisor                                ==
;==     Outputs:        DX;AX   Quotient                               ==
;==                     CX;BX   Remainder (same sign as dividend)      ==
;==     Volatile:       none                                           ==
;========================================================================

        modstart        i4d

        xdefp   __I4D

        defp    __I4D
        or      dx,dx           ; check sign of dividend
        js      divneg          ; handle case where dividend < 0
        or      cx,cx           ; check sign of divisor
        jns     __U4D           ; jump if positive  24-jan-00

;       js      notU4D          ; easy case if it is also positive

;       ; dividend >= 0, divisor >= 0
;       docall  __U4D           ; - ...
;       ret                     ; - ...

        ; dividend >= 0, divisor < 0
notU4D: neg     cx              ; take positive value of divisor
        neg     bx              ; ...
        sbb     cx,0            ; ...
        docall  __U4D           ; do unsigned division
        neg     dx              ; negate quotient
        neg     ax              ; ...
        sbb     dx,0            ; ...
        ret                     ; and return

divneg:                         ; dividend is negative
        neg     dx              ; take absolute value of dividend
        neg     ax              ; ...
        sbb     dx,0            ; ...
        or      cx,cx           ; check sign of divisor
        jns     negres          ; negative result if divisor > 0

        ; dividend < 0, divisor < 0
        neg     cx              ; negate divisor too
        neg     bx              ; ...
        sbb     cx,0            ; ...
        docall  __U4D           ; and do unsigned division
        neg     cx              ; negate remainder
        neg     bx              ; ...
        sbb     cx,0            ; ...
        ret                     ; and return

        ; dividend < 0, divisor >= 0
negres: docall  __U4D           ; do unsigned division
        neg     cx              ; negate remainder
        neg     bx              ; ...
        sbb     cx,0            ; ...
        neg     dx              ; negate quotient
        neg     ax              ; ...
        sbb     dx,0            ; ...
        ret                     ; and return

        endproc __I4D

;========================================================================
;==     Name:           U4D                                            ==
;==     Operation:      Unsigned 4 byte divide                         ==
;==     Inputs:         DX;AX   Dividend                               ==
;==                     CX;BX   Divisor                                ==
;==     Outputs:        DX;AX   Quotient                               ==
;==                     CX;BX   Remainder                              ==
;==     Volatile:       none                                           ==
;========================================================================

        xdefp   __U4D

        defp    __U4D
        or      cx,cx           ; check for easy case
        jne     noteasy         ; easy if divisor is 16 bit
        dec     bx              ; decrement divisor
        _if     ne              ; if not dividing by 1
          inc   bx              ; - put divisor back
          cmp   bx,dx           ; - if quotient will be >= 64K
          _if   be              ; - then
;
;       12-aug-88, added thanks to Eric Christensen from Fox Software
;       divisor < 64K, dividend >= 64K, quotient will be >= 64K
;
;       *note* this sequence is used in ltoa's #pragmas; any bug fixes
;              should be reflected in ltoa's code bursts
;
            mov   cx,ax         ; - - save low word of dividend
            mov   ax,dx         ; - - get high word of dividend
            sub   dx,dx         ; - - zero high part
            div   bx            ; - - divide bx into high part of dividend
            xchg  ax,cx         ; - - swap high part of quot,low word of dvdnd
          _endif                ; - endif
          div   bx              ; - calculate low part
          mov   bx,dx           ; - get remainder
          mov   dx,cx           ; - get high part of quotient
          sub   cx,cx           ; - zero high part of remainder
        _endif                  ; endif
        ret                     ; return


noteasy:                        ; have to work to do division
;
;       check for divisor > dividend
;
        _guess                  ; guess: divisor > dividend
          cmp   cx,dx           ; - quit if divisor <= dividend
          _quif b               ; - . . .
          _if   e               ; - if high parts are the same
            cmp   bx,ax         ; - - compare the lower order words
            _if   be            ; - - if divisor <= dividend
              sub   ax,bx       ; - - - calulate remainder
              mov   bx,ax       ; - - - ...
              sub   cx,cx       ; - - - ...
              sub   dx,dx       ; - - - quotient = 1
              mov   ax,1        ; - - - ...
              ret               ; - - - return
            _endif              ; - - endif
          _endif                ; - endif
          sub   cx,cx           ; - set divisor = 0 (this will be quotient)
          sub   bx,bx           ; - ...
          xchg  ax,bx           ; - return remainder = dividend
          xchg  dx,cx           ; - and quotient = 0
          ret                   ; - return
        _endguess               ; endguess
;;;        push    bp              ; save work registers
;;;        push    si              ; ...
;;;        push    di              ; ...
;;;        sub     si,si           ; zero quotient
;;;        mov     di,si           ; ...
;;;        mov     bp,si           ; and shift count
;;;moveup:                         ; loop until divisor > dividend
;;;          _shl    bx,1          ; - divisor *= 2
;;;          _rcl    cx,1          ; - ...
;;;          jc      backup        ; - know its bigger if carry out
;;;          inc     bp            ; - increment shift count
;;;          cmp     cx,dx         ; - check if its bigger yet
;;;          jb      moveup        ; - no, keep going
;;;          ja      divlup        ; - if below, know we're done
;;;          cmp     bx,ax         ; - check low parts (high parts equal)
;;;          jbe     moveup        ; until divisor > dividend
;;;divlup:                         ; division loop
;;;        clc                     ; clear carry for rotate below
;;;        _loop                   ; loop
;;;          _loop                 ; - loop
;;;            _rcl  si,1          ; - - shift bit into quotient
;;;            _rcl  di,1          ; - - . . .
;;;            dec   bp            ; - - quif( -- shift < 0 ) NB carry not changed
;;;            js    donediv       ; - - ...
;;;backup:                         ; - - entry to remove last shift
;;;            rcr   cx,1          ; - - divisor /= 2 (NB also used by 'backup')
;;;            rcr   bx,1          ; - - ...
;;;            sub   ax,bx         ; - - dividend -= divisor
;;;            sbb   dx,cx         ; - - c=1 iff it won't go
;;;            cmc                 ; - - c=1 iff it will go
;;;          _until  nc            ; - until it won't go
;;;          _loop                 ; - loop
;;;            _shl  si,1          ; - - shift 0 into quotient
;;;            _rcl  di,1          ; - - . . .
;;;            dec   bp            ; - - going to add, check if done
;;;            js    toomuch       ; - - if done, we subtracted to much
;;;            shr   cx,1          ; - - divisor /= 2
;;;            rcr   bx,1          ; - - ...
;;;            add   ax,bx         ; - - dividend += divisor
;;;            adc   dx,cx         ; - - c = 1 iff bit of quotient should be 1
;;;          _until  c             ; - until divisor will go into dividend
;;;        _endloop                ; endloop
;;;toomuch:                        ; we subtracted too much
;;;        add     ax,bx           ; dividend += divisor
;;;        adc     dx,cx           ; ...
;;;donediv:                        ; now quotient in di;si, remainder in dx;ax
;;;        mov     bx,ax           ; move remainder to cx;bx
;;;        mov     cx,dx           ; ...
;;;        mov     ax,si           ; move quotient to dx;ax
;;;        mov     dx,di           ; ...
;;;        pop     di              ; restore registers
;;;        pop     si              ; ...
;;;        pop     bp              ; ...
;;;        ret                     ; and return
; SJHowe 24-01-2000
;
; At this point here what is known is that cx > 0 and dx > cx. At the very
; least cx is 1 and dx 2.
;
; Consider the quotient
;
; The maximum it can be is when division is
;
; FFFF:FFFF / 0001:0000
;
; The minimum it can be is when division is
;
; 0002:0000 / 0001:FFFF
;
; Doing the division reveals the quotient lies 1 between FFFF. It cannot
; exceed FFFF. Therefore there is no need to keep track of the quotient's
; high word, it is always 0.
;
; Accordingly register DI has been eliminated below
;
; Should make algoritm a little faster.
;
; SJHowe 24-01-2000

        push    bp              ; save work registers
        push    si              ; ...
        sub     si,si           ; zero quotient
        mov     bp,si           ; and shift count
moveup:                         ; loop until divisor > dividend
          _shl    bx,1          ; - divisor *= 2
          _rcl    cx,1          ; - ...
          jc      backup        ; - know its bigger if carry out
          inc     bp            ; - increment shift count
          cmp     cx,dx         ; - check if its bigger yet
          jb      moveup        ; - no, keep going
          ja      divlup        ; - if below, know we're done
          cmp     bx,ax         ; - check low parts (high parts equal)
          jbe     moveup        ; until divisor > dividend
divlup:                         ; division loop
        clc                     ; clear carry for rotate below
        _loop                   ; loop
          _loop                 ; - loop
            _rcl  si,1          ; - - shift bit into quotient
            dec   bp            ; - - quif( -- shift < 0 ) NB carry not changed
            js    donediv       ; - - ...
backup:                         ; - - entry to remove last shift
            rcr   cx,1          ; - - divisor /= 2 (NB also used by 'backup')
            rcr   bx,1          ; - - ...
            sub   ax,bx         ; - - dividend -= divisor
            sbb   dx,cx         ; - - c=1 iff it won't go
            cmc                 ; - - c=1 iff it will go
          _until  nc            ; - until it won't go
          _loop                 ; - loop
            _shl  si,1          ; - - shift 0 into quotient
            dec   bp            ; - - going to add, check if done
            js    toomuch       ; - - if done, we subtracted to much
            shr   cx,1          ; - - divisor /= 2
            rcr   bx,1          ; - - ...
            add   ax,bx         ; - - dividend += divisor
            adc   dx,cx         ; - - c = 1 iff bit of quotient should be 1
          _until  c             ; - until divisor will go into dividend
        _endloop                ; endloop
toomuch:                        ; we subtracted too much
        add     ax,bx           ; dividend += divisor
        adc     dx,cx           ; ...
donediv:                        ; now quotient in si, remainder in dx;ax
        mov     bx,ax           ; move remainder to cx;bx
        mov     cx,dx           ; ...
        mov     ax,si           ; move quotient to dx;ax
        xor     dx,dx           ; ...
        pop     si              ; restore registers
        pop     bp              ; ...
        ret                     ; and return
        endproc __U4D

        endmod
        end
