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
;==     Name:           FSN                                            ==
;==     Operation:      Floating single negate                         ==
;==     Inputs:         DX      high word of float                     ==
;==     Outputs:        DX      new high word of float                 ==
;==     Volatile:       none                                           ==
;========================================================================
include mdef.inc
include struct.inc

        modstart        fsn086

        xdefp   __FSN

        defpe   __FSN
        or      ax,ax           ; if number is 0
        _if     e               ; then
          or    dx,dx           ; - check other word
        _endif                  ; endif
        _if     ne              ; if number not zero
        xor     dh,80h          ; - flip the sign bit
        _endif                  ; endif
        ret                     ; and return!!!
        endproc __FSN

        endmod
        end
