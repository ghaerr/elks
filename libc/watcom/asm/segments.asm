;*****************************************************************************
;*
;*                            Open Watcom Project
;*
;* Copyright (c) 2002-2017 The Open Watcom Contributors. All Rights Reserved.
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
;* Description:  DOS 16-bit startup code.
;*
;*****************************************************************************


;       This must be assembled using one of the following commands:
;               wasm cstrt086 -bt=DOS -mt -0r       (tiny model)
;               wasm cstrt086 -bt=DOS -ms -0r
;               wasm cstrt086 -bt=DOS -mm -0r
;               wasm cstrt086 -bt=DOS -mc -0r
;               wasm cstrt086 -bt=DOS -ml -0r
;               wasm cstrt086 -bt=DOS -mh -0r
;

include langenv.inc
include mdef.inc
include xinit.inc

include exitwmsg.inc

; PSP offsets
MEMTOP      equ 2
ENVIRON     equ 2Ch
CMDLDATA    equ 81h

FLG_NO87    equ 1
FLG_LFN     equ 100h

.286p

        name    cstart

        assume  nothing


if _MODEL and _TINY
 DGROUP group _TEXT,CONST,STRINGS,_DATA,DATA,XIB,XI,XIE,YIB,YI,YIE,_BSS
else
 DGROUP group _NULL,_AFTERNULL,CONST,STRINGS,_DATA,DATA,XIB,XI,XIE,YIB,YI,YIE,_BSS,STACK
endif

if ( _MODEL and ( _TINY or _BIG_CODE )) eq 0

; this guarantees that no function pointer will equal NULL
; (WLINK will keep segment 'BEGTEXT' in front)
; This segment must be at least 4 bytes in size to avoid confusing the
; signal function.
; need a symbol defined here to prevent the dead code elimination from
; eliminating the segment.
; (the int 3h is useful for quickly revealing jumps to NULL code pointers)

BEGTEXT  segment word public 'CODE'
        assume  cs:BEGTEXT
forever label   near
        int     3h
        jmp short forever
        public ___begtext
___begtext label byte
        nop
        nop
        nop
        nop
        assume  cs:nothing
BEGTEXT  ends

endif

_TEXT   segment word public 'CODE'

        extrn   __CMain                 : proc
        extrn   __InitRtns              : proc
        extrn   __FiniRtns              : proc

if ( _MODEL and _TINY ) eq 0
FAR_DATA segment byte public 'FAR_DATA'
FAR_DATA ends
endif

        assume  ds:DGROUP

if ( _MODEL and _TINY ) eq 0
        INIT_VAL        equ 0101h
        NUM_VAL         equ 16

_NULL   segment para public 'BEGDATA'
__nullarea label word
        dw      NUM_VAL dup(INIT_VAL)
        public  __nullarea
_NULL   ends

_AFTERNULL segment word public 'BEGDATA'
        dw      0                       ; nullchar for string at address 0
_AFTERNULL ends

endif

CONST   segment word public 'DATA'
CONST   ends

STRINGS segment word public 'DATA'
STRINGS ends

_DATA   segment word public 'DATA'

        extrn   "C",_curbrk             : word
        extrn   "C",_psp                : word
        extrn   "C",_osmajor            : byte
        extrn   "C",_osminor            : byte
        extrn   "C",_osmode             : byte
        extrn   "C",_HShift             : byte
        extrn   "C",_STACKLOW           : word
        extrn   "C",_STACKTOP           : word
        extrn   "C",_cbyte              : word
        extrn   __child                 : word
        extrn   __no87                  : byte
        extrn   "C",__uselfn            : byte
        extrn   "C",__FPE_handler       : dword
        extrn   "C",_LpCmdLine          : dword
        extrn   "C",_LpPgmName          : dword
        extrn   __get_ovl_stack         : word
        extrn   __restore_ovl_stack     : word
        extrn   __close_ovl_file        : word
        extrn   __DOSseg__              : byte
if _MODEL and _TINY
        extrn   __stacksize             : word
endif

if _MODEL and _BIG_CODE
;       Variables filled in by Microsoft Overlay Manager
;       These are here for people who want to link with Microsoft Linker
;       and use CodeView for debugging overlayed programs.
__ovlflag  db 0                 ; non-zero => program is overlayed
__intno    db 0                 ; interrupt number used by MS Overlay Manager
__ovlvec   dd 0                 ; saved contents of interrupt vector used
        public  __ovlflag
        public  __intno
        public  __ovlvec
endif

_DATA   ends

DATA    segment word public 'DATA'
DATA    ends

_BSS    segment word public 'BSS'

        extrn   _edata                  : byte  ; end of DATA (start of BSS)
        extrn   _end                    : byte  ; end of BSS (start of STACK)

_BSS    ends

if ( _MODEL and _TINY ) eq 0
STACK_SIZE      equ     800h

STACK   segment para stack 'STACK'
        db      (STACK_SIZE) dup(?)
STACK   ends
endif

        assume  nothing
        public  _cstart_

        assume  cs:_TEXT

if _MODEL and _TINY
        org     0100h
endif

_cstart_ proc near
        jmp     around

if ( _MODEL and ( _TINY or _BIG_CODE )) eq 0
        dw      ___begtext              ; make sure dead code elimination
                                        ; doesn't kill BEGTEXT segment
endif

;
; miscellaneous code-segment messages
;
if ( _MODEL and _TINY ) eq 0

NullAssign      db      '*** NULL assignment detected',0

endif

NoMemory        db      'Not enough memory',0
ConsoleName     db      'con',00h
NewLine         db      0Dh,0Ah

around: sti                             ; enable interrupts
if _MODEL and _TINY
        assume  ds:DGROUP

        mov     cx,cs
else
        mov     cx,DGROUP               ; get proper stack segment
endif

        assume  es:DGROUP

        mov     es,cx                   ; point to data segment
        mov     bx,offset DGROUP:_end   ; get bottom of stack
        add     bx,0Fh                  ; ...
        and     bl,0F0h                 ; ...
        mov     _STACKLOW,bx            ; ...
        mov     _psp,ds                 ; save segment address of PSP

if _MODEL and _TINY
        mov     ax,__stacksize          ; get size of stack required
        cmp     ax,0800h                ; make sure stack size is at least
        jae     stackok                 ; 2048 bytes
        mov     ax,0800h                ; - set stack size to 2048 bytes
stackok:
        add     bx,ax                   ; calc top address for stack
else
        add     bx,sp                   ; calculate top address for stack
endif
        add     bx,0Fh                  ; round up to paragraph boundary
        and     bl,0F0h                 ; ...
        mov     ss,cx                   ; set stack segment
        mov     sp,bx                   ; set sp relative to DGROUP
        mov     _STACKTOP,bx            ; set stack top

        mov     dx,bx                   ; make sure enough memory for stack
        shr     dx,1                    ; calc # of paragraphs needed
        shr     dx,1                    ; ... for data segment
        shr     dx,1                    ; ...
        shr     dx,1                    ; ...
;
;  check to see if running in protect-mode (Ergo 286 DPMI DOS-extender)
;
        cmp     byte ptr _osmode,0      ; if not protect-mode
        jne     mem_setup               ; then it is real-mode
        mov     cx,ds:[MEMTOP]          ; get highest segment address
        mov     ax,es                   ; point to data segment
        sub     cx,ax                   ; calc # of paragraphs available
        cmp     dx,cx                   ; compare with what we need
        jb      enuf_mem                ; if not enough memory
        mov     bx,1                    ; - set exit code
        mov     ax,offset NoMemory      ;
        mov     dx,cs                   ;
        jmp     __fatal_runtime_error   ; - display msg and exit
        ; never return

enuf_mem:                               ; endif
        mov     ax,es                   ; point to data segment
;
; This will be done by the call to _nheapgrow() in cmain.c
;if _MODEL and (_BIG_DATA or _HUGE_DATA)
;        cmp     cx,1000h                ; if more than 64K available
;        jbe     lessthan64k             ; then
;        mov     cx,1000h                ; - keep 64K for data segment
;lessthan64k:                            ; endif
;        mov     dx,cx                   ; get # of paragraphs to keep
;endif
        mov     bx,dx                   ; get # of paragraphs in data segment
        shl     bx,1                    ; calc # of bytes
        shl     bx,1                    ; ...
        shl     bx,1                    ; ...
        shl     bx,1                    ; ...
        jne     not64k                  ; if 64K
        mov     bx,0fffeh               ; - set _curbrk to 0xfffe
not64k:                                 ; endif
        mov     _curbrk,bx              ; set top of memory owned by process
        mov     bx,dx                   ; get # of paragraphs in data segment
        add     bx,ax                   ; plus start of data segment
        mov     ax,_psp                 ; get segment addr of PSP
        mov     es,ax                   ; place in ES
;
;       free up memory beyond the end of the stack in small data models
;               and beyond the 64K data segment in large data models
;
        sub     bx,ax                   ; calc # of para's we want to keep
        mov     ah,4ah                  ; "SETBLOCK" func
        int     21h                     ; free up the memory
mem_setup:
;
;       copy command line into bottom of stack
;
        mov     di,ds                   ; point es to PSP
        mov     es,di                   ; ...
        mov     di,CMDLDATA             ; DOS command buffer _psp:80
        mov     cl,-1[di]               ; get length of command
        xor     ch,ch
        cld                             ; set direction forward
        mov     al,' '
        repe    scasb
        lea     si,-1[di]

if _MODEL and _TINY
        mov     dx,cs
else
        mov     dx,DGROUP
endif
        mov     es,dx                   ; es:di is destination
        mov     di,_STACKLOW
        mov     word ptr _LpCmdLine+0,di ; stash lpCmdLine pointer
        mov     word ptr _LpCmdLine+2,es ; ...
        je      noparm
        inc     cx
        rep     movsb
noparm: sub     al,al
        stosb                           ; store NULLCHAR
        stosb                           ; assume no pgm name, store NULLCHAR
        dec     di                      ; back up pointer 1
;
;       get DOS version number
;
        mov     ah,30h
        int     21h
        mov     _osmajor,al
        mov     _osminor,ah
        mov     cx,di                   ; remember address of pgm name
        cmp     al,3                    ; if DOS version 3 or higher
        jb      nopgmname               ; then
;
;       copy the program name into bottom of stack
;
        mov     ds,ds:[ENVIRON]         ; get segment addr of environment area
        sub     si,si                   ; offset 0
        mov     bp,FLG_LFN              ; NO87 not present and LFN=N not present!
L1:     mov     ax,[si]                 ; get first part of environment var
        or      ax,2020H                ; lower case
        cmp     ax,"on"                 ; if first part is 'NO'
        jne     L2                      ; then
        cmp     word ptr [si+2],"78"    ; if second part is '87'
        jne     L3                      ; then
        cmp     byte ptr [si+4],"="     ; if third part is '='
        jne     L3                      ; then
        or      bp,FLG_NO87             ; set bp to indicate NO87
        jmp     L3                      ; -
L2:     cmp     ax,"fl"                 ; if first part is 'LF'
        jne     L4                      ; then
        mov     ax,2[si]                ; get second part
        or      al,20h                  ; lower case
        cmp     ax,"=n"                 ; if second part is 'N='
        jne     L4                      ; then
        mov     al,4[si]                ; get third part
        or      al,20h                  ; lower case
        cmp     al,"n"                  ; if third part is 'N'
        jne     L4                      ; then
        and     bp,not FLG_LFN          ; set bp to indicate no LFN
L3:     add     si,5                    ; skip 'NO87=' or 'LFN=N'
L4:     cmp     byte ptr [si],0         ; end of string ?
        lodsb
        jne     L4                      ; until end of string
        cmp     byte ptr [si],0         ; end of all strings ?
        jne     L1                      ; if not, then skip next string
        lodsb
        inc     si                      ; - point to program name
        inc     si                      ; - . . .
L5:     cmp     byte ptr [si],0         ; - end of pgm name ?
        movsb                           ; - copy a byte
        jne     L5                      ; - until end of pgm name
nopgmname:                              ; endif

        assume  ds:DGROUP

        mov     ds,dx
        mov     si,cx                   ; save address of pgm name
        mov     word ptr _LpPgmName+0,si ; stash LpPgmName pointer
        mov     word ptr _LpPgmName+2,es ; ...
        mov     bx,sp                   ; end of stack in data segment
        mov     ax,bp
        mov     __no87,al               ; set state of "NO87" environment var
        and     __uselfn,ah             ; set "LFN" support status
        mov     _STACKLOW,di            ; save low address of stack

        mov     cx,offset DGROUP:_end   ; end of _BSS segment (start of STACK)
        mov     di,offset DGROUP:_edata ; start of _BSS segment
        sub     cx,di                   ; calc # of bytes in _BSS segment
        xor     al,al                   ; zero the _BSS segment
        rep     stosb                   ; . . .

        cmp     word ptr __get_ovl_stack,0 ; if program not overlayed
        jne     _is_ovl                 ; then
        mov     ax,offset __null_ovl_rtn; - set vectors to null rtn
        mov     __get_ovl_stack,ax      ; - ...
        mov     __get_ovl_stack+2,cs    ; - ...
        mov     __restore_ovl_stack,ax  ; - ...
        mov     __restore_ovl_stack+2,cs; - ...
        mov     __close_ovl_file,ax     ; - ...
        mov     __close_ovl_file+2,cs   ; - ...
_is_ovl:                                ; endif
        xor     bp,bp                   ; set up stack frame
if _MODEL and _BIG_CODE
        push    bp                      ; ... for new overlay manager
        mov     bp,sp                   ; ...
endif
        ; DON'T MODIFY BP FROM THIS POINT ON!
        mov     ax,offset __null_FPE_rtn; initialize floating-point exception
        mov     word ptr __FPE_handler,ax       ; ... handler address
        mov     word ptr __FPE_handler+2,cs     ; ...

        mov     ax,0FFh                 ; run all initalizers
        call    __InitRtns              ; call initializer routines
        call    __CMain
_cstart_ endp

;       don't touch AL in __exit, it has the return code

__exit  proc
        public  "C",__exit
        push    ax                      ; save return code on stack
if _MODEL and _TINY
        jmp short L7
else
        mov     dx,DGROUP
        mov     ds,dx
        cld                             ; check lower region for altered values
        lea     di,__nullarea           ; set es:di for scan
        mov     es,dx
        mov     cx,NUM_VAL
        mov     ax,INIT_VAL
        repe    scasw
        je      L7
;
; low memory has been altered
;
        pop     bx                      ; restore return code
        mov     ax,offset NullAssign    ; point to msg
        mov     dx,cs                   ; . . .
endif

        public  __do_exit_with_msg_

; input: ( char __far *msg, int rc ) always in registers
;       DX:AX - far pointer to message to print
;       BX    - exit code

__do_exit_with_msg_:
        mov     sp,offset DGROUP:_end+80h; set a good stack pointer
        push    bx                      ; save return code
        push    ax                      ; save address of msg
        push    dx                      ; . . .
        mov     di,cs
        mov     ds,di
        mov     dx,offset ConsoleName
        mov     ax,03d01h               ; write-only access to screen
        int     021h
        mov     bx,ax                   ; get file handle
        pop     ds                      ; restore address of msg
        pop     dx                      ; . . .
        mov     si,dx                   ; get address of msg
        cld                             ; make sure direction forward
L6:     lodsb                           ; get char
        test    al,al                   ; end of string?
        jne     L6                      ; no
        mov     cx,si                   ; calc length of string
        sub     cx,dx                   ; . . .
        dec     cx                      ; . . .
        mov     ah,040h                 ; write out the string
        int     021h                    ; . . .
        mov     ds,di
        mov     dx,offset NewLine       ; write out the new line
        mov     cx,sizeof NewLine       ; . . .
        mov     ah,040h                 ; . . .
        int     021h                    ; . . .
L7:
if _MODEL and _BIG_CODE
        mov     dx,DGROUP               ; get access to DGROUP
        mov     ds,dx                   ; . . .
        cmp     byte ptr __ovlflag,0    ; if MS Overlay Manager present
        je      no_ovl                  ; then
        mov     al,__intno              ; - get interrupt number used
        mov     ah,25h                  ; - DOS func to set interrupt vector
        lds     dx,__ovlvec             ; - get previous contents of vector
        int     21h                     ; - restore interrupt vector
no_ovl:                                 ; endif
endif
        xor     ax,ax                   ; run finalizers
        mov     dx,FINI_PRIORITY_EXIT-1 ; less than exit
        call    __FiniRtns              ; do finalization
        pop     ax                      ; restore return code from stack
        mov     ah,04cH                 ; DOS call to exit with return code
        int     021h                    ; back to DOS
__exit  endp

;
; copyright message
;
include msgcpyrt.inc

;
;       set up addressability without segment relocations for emulator
;
public  __GETDS
__GETDS proc    near
        push    ax                      ; save ax
if _MODEL and _TINY
;       can't have segment fixups in the TINY memory model
        mov     ax,cs                   ; DS=CS
else
        mov     ax,DGROUP               ; get DGROUP
endif
        mov     ds,ax                   ; load DS with appropriate value
        pop     ax                      ; restore ax
        ret                             ; return
__GETDS endp


__null_FPE_rtn proc far
        ret                             ; return
__null_FPE_rtn endp

__null_ovl_rtn proc far
        ret                             ; return
__null_ovl_rtn endp


_TEXT   ends

        end     _cstart_
