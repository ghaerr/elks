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


;<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
;<>
;<> __FSC compares DX:AX with CX:BX
;<>       if DX:AX > CX:BX,  1 is returned in AX
;<>       if DX:AX = CX:BX,  0 is returned in AX
;<>       if DX:AX < CX:BX, -1 is returned in AX
;<>
;<>  =========    ===           =======
;<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
include mdef.inc
include struct.inc

        modstart        fsc086

        xdefp   __FSC

        defpe   __FSC
        _guess    have_cmp      ; guess - comparison done
          xor     DX,CX         ; - check if signs differ
          _if     ns            ; - if signs are the same
            xor     DX,CX       ; - - restore DX
            sub     DX,CX       ; - - find difference, DX=0 if equal
            _if     e           ; - - if equal
              cmp     AX,BX     ; - - - compare mantissas
              mov     AX,0      ; - - - assume equal
              _quif   e,have_cmp  ; - done if mantissas also equal
            _endif              ; - - endif
                                ; - - carry=1 iff |DX:AX| < |CX:BX|
            rcr     DX,1        ; - - DX sign set iff |DX:AX| < |CX:BX|
            xor     CX,DX       ; - - CX sign set iff DX:AX < CX:BX
            not     CX          ; - - CX sign set iff DX:AX > CX:BX
          _endif                ; - endif
          sub     AX,AX         ; - clear result
          _shl    CX,1          ; - carry=1 iff DX:AX > CX:BX
          adc     AX,AX         ; - AX = 1 iff DX:AX > CX:BX, else AX=0
          _shl    AX,1          ; - AX = 2 iff DX:AX > CX:BX, else AX=0
          dec     AX            ; - AX = 1 iff DX:AX > CX:BX, else AX=-1
        _endguess               ; endguess
        ret                     ; return with sign in AX, conditions set
        endproc __FSC

        endmod
        end
