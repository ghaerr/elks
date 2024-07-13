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
;==     Name:           I8M,U8M                                        ==
;==     Operation:      integer eight byte multiply                    ==
;==     Inputs:         AX:BX:CX:DX integer M1                         ==
;==                     [SS:SI]     integer M2                         ==
;==     Outputs:        AX:BX:CX:DX product                            ==
;========================================================================
include mdef.inc
include struct.inc

        modstart        i8m086

; here is the general case:
;
; si6 si4 si2 si0
; ax  bx  cx  dx
;                                                             hi:dx*si0 lo:dx*si0
;                                                   hi:dx*si2 lo:dx*si2
;                                         hi:dx*si4 lo:dx*si4
;                               hi:dx*si6 lo:dx*si6
;                                                   hi:cx*si0 lo:cx*si0
;                                         hi:cx*si2 lo:cx*si2
;                               hi:cx*si4 lo:cx*si4
;                     hi:cx*si6 lo:cx*si6
;                                         hi:bx*si0 lo:bx*si0
;                               hi:bx*si2 lo:bx*si2
;                     hi:bx*si4 lo:bx*si4
;           hi:bx*si6 lo:bx*si6
;                               hi:ax*si0 lo:ax*si0
;                     hi:ax*si2 lo:ax*si2
;           hi:ax*si4 lo:ax*si4
; hi:ax*si6 lo:ax*si6
;
; here is everything laid out for execution...
;
;                                                             hi:dx*si0 lo:dx*si0
;                                                   hi:dx*si2 lo:dx*si2
;                                                   hi:cx*si0 lo:cx*si0
;                                         hi:dx*si4 lo:dx*si4
;                                         hi:cx*si2 lo:cx*si2
;                                         hi:bx*si0 lo:bx*si0
;                               hi:dx*si6 lo:dx*si6
;                               hi:cx*si4 lo:cx*si4
;                               hi:bx*si2 lo:bx*si2
;                               hi:ax*si0 lo:ax*si0
;                     hi:cx*si6 lo:cx*si6
;                     hi:bx*si4 lo:bx*si4
;                     hi:ax*si2 lo:ax*si2
;           hi:bx*si6 lo:bx*si6
;           hi:ax*si4 lo:ax*si4
; hi:ax*si6 lo:ax*si6
;
; remove the high order words...
;
;                     hi:dx*si0 lo:dx*si0
;           hi:dx*si2 lo:dx*si2
;           hi:cx*si0 lo:cx*si0
; hi:dx*si4 lo:dx*si4
; hi:cx*si2 lo:cx*si2
; hi:bx*si0 lo:bx*si0
; lo:dx*si6
; lo:cx*si4
; lo:bx*si2
; lo:ax*si0
; == ax ==  == bx ==  == cx ==   == dx ==
;
oax     equ     -2
obx     equ     -4
ocx     equ     -6
odx     equ     -8

        xdefp   __I8M
        xdefp   __U8M
        xdefp   __I8ME
        xdefp   __U8ME

        defp    __U8ME
        defp    __I8ME
        push    si
        push    es:6[si]
        push    es:4[si]
        push    es:2[si]
        push    es:[si]
        mov     si,sp
        lcall   __U8M
        add     sp,4*2
        pop     si
        ret
        endproc __I8ME
        endproc __U8ME

        defp    __U8M
        defp    __I8M
        push    bp
        mov     bp,sp
        push    ax              ; oax[bp]
        push    bx              ; obx[bp]
        push    cx              ; ocx[bp]
        push    dx              ; odx[bp]
        or      ax,ss:6[si]
        je      l1
        jmp     u8mu8
l1:     or      bx,ss:4[si]
        jne     u6mu6
        or      cx,ss:2[si]
        jne     u4mu4
        or      dx,ss:[si]
        jne     u2mu2
        ; zero times zero here
        mov     sp,bp
        pop     bp
        ret
u2mu2           label   near
;
;
;                     hi:dx*si0 lo:dx*si0
;           hi:dx*si2 lo:dx*si2
;           hi:cx*si0 lo:cx*si0
; hi:dx*si4 lo:dx*si4
; hi:cx*si2 lo:cx*si2
; hi:bx*si0 lo:bx*si0
; lo:dx*si6
; lo:cx*si4
; lo:bx*si2
; lo:ax*si0
; == ax ==  == bx ==  == cx ==   == dx ==
;
; cancel entries with: ax=0 bx=0 cx=0 si6=0 si4=0 si2=0
;
;                     hi:dx*si0 lo:dx*si0
; == ax ==  == bx ==  == cx ==   == dx ==
        mov     ax,odx[bp]
        mul     word ptr ss:[si]        ; dx*si0
        xchg    ax,dx
        xchg    ax,cx
        mov     sp,bp
        pop     bp
        ret
u4mu4           label   near
;
;
;                     hi:dx*si0 lo:dx*si0
;           hi:dx*si2 lo:dx*si2
;           hi:cx*si0 lo:cx*si0
; hi:dx*si4 lo:dx*si4
; hi:cx*si2 lo:cx*si2
; hi:bx*si0 lo:bx*si0
; lo:dx*si6
; lo:cx*si4
; lo:bx*si2
; lo:ax*si0
; == ax ==  == bx ==  == cx ==   == dx ==
;
; cancel entries with: ax=0 bx=0 si6=0 si4=0
;
;                     hi:dx*si0 lo:dx*si0       (1)
;           hi:dx*si2 lo:dx*si2                 (3)
;           hi:cx*si0 lo:cx*si0                 (4)
; hi:cx*si2 lo:cx*si2                           (2)
; == ax ==  == bx ==  == cx ==   == dx ==
;  -16[bp]   -14[bp]   -12[bp]    -10[bp]
        mov     ax,odx[bp]      ; (1)
        mul     word ptr ss:[si]        ; dx*si0
        push    ax              ; lo:dx*si0             -10[bp]
        push    dx              ; hi:dx*si0             -12[bp]
        mov     ax,ocx[bp]      ; (2)
        mul     word ptr ss:2[si]       ; cx*si2
        push    ax              ; lo:cx*si2             -14[bp]
        push    dx              ; hi:cx*si2             -16[bp]
        mov     ax,odx[bp]      ; (3)
        mul     word ptr ss:2[si]       ; dx*si2
        add     -12[bp],ax
        adc     -14[bp],dx
        adc     -16[bp],bx
        mov     ax,ocx[bp]      ; (4)
        mul     word ptr ss:[si]        ; cx*si0
        add     -12[bp],ax
        adc     -14[bp],dx
        adc     -16[bp],bx
        pop     ax
        pop     bx
        pop     cx
        pop     dx
        mov     sp,bp
        pop     bp
        ret
u6mu6           label   near
;                     hi:dx*si0 lo:dx*si0
;           hi:dx*si2 lo:dx*si2
;           hi:cx*si0 lo:cx*si0
; hi:dx*si4 lo:dx*si4
; hi:cx*si2 lo:cx*si2
; hi:bx*si0 lo:bx*si0
; lo:dx*si6
; lo:cx*si4
; lo:bx*si2
; lo:ax*si0
; == ax ==  == bx ==  == cx ==   == dx ==
;
; cancel entries with: ax=0 si6=0
;
;                     hi:dx*si0 lo:dx*si0       (1)
;           hi:dx*si2 lo:dx*si2                 (3)
;           hi:cx*si0 lo:cx*si0                 (4)
; hi:dx*si4 lo:dx*si4                           (2)
; hi:cx*si2 lo:cx*si2                           (5)
; hi:bx*si0 lo:bx*si0                           (6)
; lo:cx*si4                                     (7)
; lo:bx*si2                                     (8)
; == ax ==  == bx ==  == cx ==   == dx ==
;  -16[bp]   -14[bp]   -12[bp]    -10[bp]
;
        mov     ax,odx[bp]      ; (1)
        mul     word ptr ss:[si]        ; dx*si0
        push    ax              ; lo:dx*si0     -10[bp]
        push    dx              ; hi:dx*si0     -12[bp]
        mov     ax,odx[bp]      ; (2)
        mul     word ptr ss:4[si]       ; dx*si4
        push    ax              ; lo:dx*si4     -14[bp]
        push    dx              ; hi:dx*si4     -16[bp]
        mov     ax,odx[bp]      ; (3)
        mul     word ptr ss:2[si]       ; dx*si2
        add     -12[bp],ax
        adc     -14[bp],dx
        adc     word ptr -16[bp],0
        mov     ax,ocx[bp]      ; (4)
        mul     word ptr ss:[si]        ; cx*si0
        add     -12[bp],ax
        adc     -14[bp],dx
        adc     word ptr -16[bp],0
        mov     ax,ocx[bp]      ; (5)
        mul     word ptr ss:2[si]       ; cx*si2
        add     -14[bp],ax
        adc     -16[bp],dx
        mov     ax,obx[bp]      ; (6)
        mul     word ptr ss:[si]        ; bx*si0
        add     -14[bp],ax
        adc     -16[bp],dx
        mov     ax,ocx[bp]      ; (7)
        mul     word ptr ss:4[si]       ; cx*si4
        add     -16[bp],ax
        mov     ax,obx[bp]      ; (8)
        mul     word ptr ss:2[si]       ; bx*si2
        add     -16[bp],ax
        pop     ax
        pop     bx
        pop     cx
        pop     dx
        mov     sp,bp
        pop     bp
        ret
u8mu8           label   near
;
;
;                     hi:dx*si0 lo:dx*si0       (1)
;           hi:dx*si2 lo:dx*si2                 (3)
;           hi:cx*si0 lo:cx*si0                 (4)
; hi:dx*si4 lo:dx*si4                           (2)
; hi:cx*si2 lo:cx*si2                           (5)
; hi:bx*si0 lo:bx*si0                           (6)
; lo:dx*si6                                     (9)
; lo:cx*si4                                     (7)
; lo:bx*si2                                     (8)
; lo:ax*si0                                     (10)
; == ax ==  == bx ==  == cx ==   == dx ==
;  -16[bp]   -14[bp]   -12[bp]    -10[bp]
;
        mov     ax,odx[bp]      ; (1)
        mul     word ptr ss:[si]        ; dx*si0
        push    ax              ; lo:dx*si0     -10[bp]
        push    dx              ; hi:dx*si0     -12[bp]
        mov     ax,odx[bp]      ; (2)
        mul     word ptr ss:4[si]       ; dx*si4
        push    ax              ; lo:dx*si4     -14[bp]
        push    dx              ; hi:dx*si4     -16[bp]
        mov     ax,odx[bp]      ; (3)
        mul     word ptr ss:2[si]       ; dx*si2
        add     -12[bp],ax
        adc     -14[bp],dx
        adc     word ptr -16[bp],0
        mov     ax,ocx[bp]      ; (4)
        mul     word ptr ss:[si]        ; cx*si0
        add     -12[bp],ax
        adc     -14[bp],dx
        adc     word ptr -16[bp],0
        mov     ax,ocx[bp]      ; (5)
        mul     word ptr ss:2[si]       ; cx*si2
        add     -14[bp],ax
        adc     -16[bp],dx
        mov     ax,obx[bp]      ; (6)
        mul     word ptr ss:[si]        ; bx*si0
        add     -14[bp],ax
        adc     -16[bp],dx
        mov     ax,ocx[bp]      ; (7)
        mul     word ptr ss:4[si]       ; cx*si4
        add     -16[bp],ax
        mov     ax,obx[bp]      ; (8)
        mul     word ptr ss:2[si]       ; bx*si2
        add     -16[bp],ax
        mov     ax,odx[bp]      ; (9)
        mul     word ptr ss:6[si]       ; dx*si6
        add     -16[bp],ax
        mov     ax,oax[bp]      ; (10)
        mul     word ptr ss:[si]        ; ax*si0
        add     -16[bp],ax
        pop     ax
        pop     bx
        pop     cx
        pop     dx
        mov     sp,bp
        pop     bp
        ret
        endproc __I8M
        endproc __U8M

        endmod
        end
