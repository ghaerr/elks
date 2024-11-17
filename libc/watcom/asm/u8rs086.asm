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
;==     Name:           U8RS                                           ==
;==     Operation:      integer eight byte right shift (unsigned)      ==
;==     Inputs:         AX:BX:CX:DX integer M1                         ==
;==                     SI          shift count                        ==
;==     Outputs:        AX:BX:CX:DX result                             ==
;========================================================================
include mdef.inc
include struct.inc

        modstart        u8rs086

        xdefp   __U8RS

        defp    __U8RS
        test    si,si                   ; shifting anything?
        _if     ne                      ; if so then
          _loop                         ; - top of loop
            shr ax,1                    ; - - shift one bit
            rcr bx,1                    ; - - ...
            rcr cx,1                    ; - - ...
            rcr dx,1                    ; - - ...
            dec si                      ; - - decrement shift count
          _until e                      ; - kick out if done
        _endif                          ; endif
        ret
        endproc __U8RS

        endmod
        end
