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
;* Description:  C library setjmp/longjmp support for x86 16-bit processors
;*
;*****************************************************************************


;; NOTE:::
;;      setjmp/longjmp not guarenteed to work in Windows REAL mode!
;;      After doing a setjmp from one segment, if you call to another
;;      segment, do something to cause the segment containing the setjmp
;;      call to be paged out or moved in memory, and then try to longjmp
;;      back, longjmp doesn't know where to jump back to.

include mdef.inc
include struct.inc

ifndef __OS2__
  ifndef __WINDOWS__
    ifndef __QNX__
      __DOING_DOS__ = 1
    endif
  endif
endif

ifdef __DOING_DOS__
        datasegment
        extrn           __get_ovl_stack : dword
        extrn           __restore_ovl_stack : dword
        enddata
endif
        codeptr         "C",__longjmp_handler

        modstart        setjmp

        xdefp   "C",_setjmp
        defpe   _setjmp
if _MODEL and (_BIG_DATA or _HUGE_DATA)
        push    DS              ; save DS
        mov     DS,DX           ; get FP_SEG(buf)
        pop     DX              ; load DS value into DX
endif
        xchg    ax,si           ; save si and get buffer addr into index reg
        mov     [si],bx         ; save registers
        mov     2[si],cx        ; ...
        mov     6[si],ax        ; save si (swapped with ax)
        mov     8[si],di        ; ...
        mov     10[si],bp       ; ...
        mov     14[si],es       ; ...
if _MODEL and (_BIG_DATA or _HUGE_DATA)
        mov     4[si],ds        ; save dx (swapped with ds)
        mov     16[si],dx       ; save ds value
else
        mov     4[si],dx        ; save dx
        mov     16[si],ds       ; save ds
endif
        pop     20[si]          ; (return address)
if _MODEL and _BIG_CODE
        pop     18[si]          ; segment part of return address
        mov     12[si],sp       ; save sp
        push    18[si]          ; push seg part of return address
else
        mov     12[si],sp       ; save sp
        mov     18[si],cs       ; segment part
endif
        push    20[si]          ; push return address again
ifdef   __DOING_DOS__
  if _MODEL and _BIG_CODE
        push    dx
        mov     dx,18[si]       ; pass segment to getovlstack
  endif
  if _MODEL and (_BIG_DATA or _HUGE_DATA)
        push    ds              ; save ds
        mov     ax,DGROUP       ; point to DGROUP       05-jan-92
        mov     ds,ax           ; ...
  endif
        mov     ax,word ptr __get_ovl_stack
        or      ax,word ptr __get_ovl_stack+2
        _if     ne              ; if routine defined
          call  __get_ovl_stack ; - get overlay stack pointer
        _endif                  ; endif
  if _MODEL and (_BIG_DATA or _HUGE_DATA)
        pop     ds              ; restore ds
  endif
  if _MODEL and _BIG_CODE
        mov     18[si],dx       ; save modified return segment.
        pop     dx
  endif
        mov     22[si],ax       ; save it
endif
        mov     24[si],ss       ; save stack segment
        mov     si,6[si]        ; restore si
if _MODEL and (_BIG_DATA or _HUGE_DATA)
        mov     ds,dx           ; restore ds
endif
        sub     ax,ax           ; return 0
        ret                     ; return
_setjmp endp



        xdefp   "C",longjmp
        defpe   longjmp
if _MODEL and (_BIG_DATA or _HUGE_DATA)
    ifdef __WINDOWS__
        mov     di,ds           ; save pegged DS
    endif
        mov     DS,DX           ; get FP_SEG(buf)
else
        mov     BX,DX           ; get return value
endif
        mov     si, ax          ; get address of save area
        mov     cx, 12[ si ]    ; setup old ss:sp as a parm
        mov     dx, 24[ si ]

if _MODEL and (_BIG_DATA or _HUGE_DATA)
        push    ds
    ifdef __WINDOWS__
        mov     ds, di
    else
        mov     ax, seg __longjmp_handler
        mov     ds, ax
    endif
endif
        mov     ax, cx
        call    __longjmp_handler       ; call handler

if _MODEL and (_BIG_DATA or _HUGE_DATA)
        pop     ds
endif

        mov     ss, 24[ si ]    ; load ss
        mov     sp, 12[ si ]    ; load sp
        or      bx, bx          ; check return value
        jne     L1              ; if 0
        inc     bx              ; - set it to 1
L1:                             ; endif
        mov     cx, 2[ si ]     ; restore some saved registers.
        mov     bp, 10[ si ]    ; ...
        mov     es, 14[ si ]    ; ...
ifdef   __DOING_DOS__
        mov     ax, 22[ si ]    ; get overlay stack pointer value
  if _MODEL and _BIG_CODE
        mov     dx, 18[ si ]    ; get seg part of return address.
  endif
  if _MODEL and (_BIG_DATA or _HUGE_DATA)
        push    ds              ; save ds
        mov     di,DGROUP       ; point to DGROUP       05-jan-92
        mov     ds,di           ; ...
  endif
        mov     di,word ptr __restore_ovl_stack
        or      di,word ptr __restore_ovl_stack+2
        _if     ne              ; if routine defined
          call  __restore_ovl_stack; - restore overlay stack (MAY MODIFY DX !)
        _endif                  ; endif
  if _MODEL and (_BIG_DATA or _HUGE_DATA)
        pop     ds              ; restore ds
  endif
  if _MODEL and _BIG_CODE
        push    dx              ; push seg part of return address.
  endif
else
  if _MODEL and _BIG_CODE
        push    18[ si ]        ; push seg part of return address
  endif
endif
        push    20[ si ]        ; push (ip)
        push    bx              ; save return value
        mov     bx, [ si ]      ; restore saved registers
        mov     di, 8[ si ]     ; ...
        mov     dx, 4[ si ]     ; restore dx now.
        mov     ax, 16[ si ]    ; get value for ds into ax
        mov     si, 6[ si ]     ; ...
        mov     ds, ax          ; restore ds
        pop     ax              ; restore return value
        ret                     ; return to point following setjmp call
longjmp endp

_TEXT   ends
        end
