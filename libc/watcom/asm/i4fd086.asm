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


; Name:         __I4FD, __U4FD
; Operation:    __I4FD convert signed 32-bit integer in DX:AX into double float
;               __U4FD convert unsigned 32-bit integer in DX:AX into double float
; Inputs:       DX:AX       - 32-bit integer
; Outputs:      AX BX CX DX - double precision representation of integer
; Volatile:     none
;
;
include mdef.inc
include struct.inc

        modstart        i4fd086

        xdefp   __I4FD
        xdefp   __U4FD


        defpe   __I4FD
        or      DX,DX           ; if number is negative
        _if     s               ; then
          not   DX              ; - negate number
          neg   AX              ; - . . .
          sbb   DX,-1           ; - . . .
          mov   CX,0BFF0h+(36*16); - set exponent
        _else                   ; else

;       convert unsigned 32-bit integer to double

        defpe   __U4FD
          mov   CX,3FF0h+(36*16); - set exponent
        _endif                  ; endif
        sub     BX,BX           ; zero registers
        or      BX,DX           ; put high order word in BX
        mov     DX,AX           ; put low order word in DX
        _if     e               ; if high order word is 0
          sub   DX,DX           ; - set DX back to 0
          or    BX,AX           ; - place low order word in BX
          _if   e               ; - if # is 0
            sub   CX,CX         ; - - zero rest
            ret                 ; - - return
          _endif                ; - endif
          sub   CX,0100h        ; - adjust exponent by 16
        _endif                  ; endif
        xchg    CX,DX           ; DX <- exp, CX <- mantissa
        sub     AX,AX           ; set rest of mantissa to 0
        cmp     BH,0            ; if high byte is 0
        _if     e               ; then
          mov   AX,BX           ; - shift left by 16
          mov   BX,CX           ; - ...
          sub   CX,CX           ; - ...
          sub   DX,0100h        ; - adjust exponent by 16
        _else                   ; else
          mov   AL,BH           ; - shift integer left by 8 bits
          mov   BH,BL           ; - . . .
          mov   BL,CH           ; - . . .
          mov   CH,CL           ; - . . .
          xor   CL,CL           ; - . . .
          sub   DX,0080h        ; - exp <-- exp - 8
        _endif                  ; endif

;       At this point AL will be non-zero, must determine if we have to shift
;       left or right.

        test    AL,0F0h         ; see if we have to shift forward or backward
        _if     e               ; if (we haven't shifted msb into bit 53)
          _guess                ; - guess: have to shift up to 4 bits
            sub   DX,0010h      ; - - exp <-- exp - 1
            _shl  CX,1          ; - - shift integer left by 1 bit
            _rcl  BX,1          ; - - . . .
            _rcl  AL,1          ; - - . . .
            test  AL,0F0h       ; - - check to see if done
            _quif ne            ; - - quit if done
            sub   DX,0010h      ; - - exp <-- exp - 1
            _shl  CX,1          ; - - shift integer left by 1 bit
            _rcl  BX,1          ; - - . . .
            _rcl  AL,1          ; - - . . .
            test  AL,0F0h       ; - - check to see if done
            _quif ne            ; - - quit if done
            sub   DX,0010h      ; - - exp <-- exp - 1
            _shl  CX,1          ; - - shift integer left by 1 bit
            _rcl  BX,1          ; - - . . .
            _rcl  AL,1          ; - - . . .
            test  AL,0F0h       ; - - check to see if done
            _quif ne            ; - - quit if done
            sub   DX,0010h      ; - - exp <-- exp - 1
            _shl  CX,1          ; - - shift integer left by 1 bit
            _rcl  BX,1          ; - - . . .
            _rcl  AL,1          ; - - . . .
          _endguess             ; - endguess
          and   AL,0Fh          ; - clear out implied bit
          or    AX,DX           ; - put in exponent
          sub   DX,DX           ; - set low word to 0
          ret                   ; - return
        _endif                  ; endif

;       we must shift to the right

          _guess                ; - guess
            test  AL,0E0h       ; - - see if done
            _quif e             ; - - quit if we are done
            add   DX,0010h      ; - - exp <-- exp + 1
            shr   AX,1          ; - - shift integer right by 1 bit
            rcr   BX,1          ; - - . . .
            rcr   CX,1          ; - - . . .
            test  AL,0E0h       ; - - see if done
            _quif e             ; - - quit if we are done
            add   DX,0010h      ; - - exp <-- exp + 1
            shr   AX,1          ; - - shift integer right by 1 bit
            rcr   BX,1          ; - - . . .
            rcr   CX,1          ; - - . . .
            test  AL,0E0h       ; - - see if done
            _quif e             ; - - quit if we are done
            add   DX,0010h      ; - - exp <-- exp + 1
            shr   AX,1          ; - - shift integer right by 1 bit
            rcr   BX,1          ; - - . . .
            rcr   CX,1          ; - - . . .
          _endguess             ; - endguess
        and     AL,0Fh          ; clear out implied bit
        or      AX,DX           ; put in exponent
        sub     DX,DX           ; set low word to 0
        ret                     ; return
        endproc __U4FD
        endproc __I4FD

        endmod
        end
