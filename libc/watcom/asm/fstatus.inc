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


FPE_OK                  equ     0
FPE_INVALID             equ     81h
FPE_DENORMAL            equ     82h
FPE_ZERODIVIDE          equ     83h
FPE_OVERFLOW            equ     84h
FPE_UNDERFLOW           equ     85h
FPE_INEXACT             equ     86h
FPE_UNEMULATED          equ     87h
FPE_SQRTNEG             equ     88h
FPE_STACKOVERFLOW       equ     8ah
FPE_STACKUNDERFLOW      equ     8bh
FPE_EXPLICITGEN         equ     8ch
FPE_IOVERFLOW           equ     8dh
FPE_LOGERR              equ     8eh
FPE_MODERR              equ     8fh
