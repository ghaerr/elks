;*****************************************************************************
;*
;*                            Open Watcom Project
;*
;* Copyright (c) 2002-2022 The Open Watcom Contributors. All Rights Reserved.
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
;* Description:  Memory model setup for interfacing with C code.
;*
;*****************************************************************************


_SMALL_CODE     =       00h
_BIG_CODE       =       01h
_SMALL_DATA     =       00h
_BIG_DATA       =       02h
_HUGE_DATA      =       04h
_LONG_INTS      =       08h

_USE_32_SEGS    =       10h
_386            =       10h     ; from old clib

_DS_PEGGED      =       20h     ; from old clib (used for Windows)
_TINY           =       40h     ; from old clib (formerly 10h)

_FLAT           =       ( _SMALL_CODE or _SMALL_DATA or _USE_32_SEGS )
_SMALL          =       ( _SMALL_CODE or _SMALL_DATA )
_COMPACT        =       ( _SMALL_CODE or _BIG_DATA )
_MEDIUM         =       ( _BIG_CODE   or _SMALL_DATA )
_LARGE          =       ( _BIG_CODE   or _BIG_DATA )
_HUGE           =       ( _BIG_CODE   or _HUGE_DATA )

_EMULATION      =       00h
_8087           =       01h

ifdef __WASM__
    ifdef __FLAT__
        _MODEL = _FLAT
    endif
    ifdef __TINY__
        _MODEL = _TINY
    endif
    ifdef __SMALL__
        _MODEL = _SMALL
    endif
    ifdef __COMPACT__
        _MODEL = _COMPACT
    endif
    ifdef __MEDIUM__
        _MODEL = _MEDIUM
    endif
    ifdef __LARGE__
        _MODEL = _LARGE
    endif
    ifdef __HUGE__
        _MODEL = _HUGE
    endif
    ifdef __386__
        _MODEL = ( _MODEL or _USE_32_SEGS )
    endif
    ifdef __WINDOWS__
        if _MODEL and (_BIG_DATA or _HUGE_DATA)
            _MODEL = ( _MODEL or _DS_PEGGED )
        endif
    endif
    ifdef __FPC__
        _MATH = _EMULATION
    endif
    ifdef __FPI__
        _MATH = _8087
    endif
    ifdef __FPI87__
        _MATH = _8087
    endif
else
;include         .\model.inc     ; defines _MODEL, _MATH symbols
.ERR
endif

if _MODEL and _BIG_CODE

modstart        macro   modname,alignment
ifdef NDEBUG
;                name    modname
endif
                ifb <alignment>
_TEXT           segment word public 'CODE'
                else
_TEXT           segment alignment public 'CODE'
                endif
                assume  cs:_TEXT
                endm

calli           macro   regname
                call    dword ptr [regname]
                endm

codeptr         macro   p1,p2
                ifb <p2>
                extrn p1 : dword
                else
                extrn "&p1",p2 : dword
                endif
                endm

docall          macro   dest
                call    far ptr dest
                endm

dojmp           macro   dest
                jmp     far ptr dest
                endm

defp            macro   dsym,exp
         dsym   proc    far exp
                endm

defpe           macro   dsym
                ifdef _EXPORT
         dsym   proc    far export
                else
         dsym   proc    far
                endif
                endm

defn            macro   dsym
         dsym   proc    near
                endm

endproc         macro   dsym
         dsym   endp
                endm

epilog          macro
                if _MODEL and _USE_32_SEGS
                    pop     EBP
                    dec     EBP
                    mov     ESP,EBP
                else
                    pop     BP
                    dec     BP
                endif
                endm

prolog          macro
                if _MODEL and _USE_32_SEGS
                    inc     EBP
                    push    EBP
                    mov     EBP,ESP
                else
                    inc     BP
                    push    BP
                    mov     BP,SP
                endif
                endm

xdefp           macro   p1,p2
                ifb <p2>
                public p1
                else
                public "&p1",p2
                endif
                endm

xrefp           macro   p1,p2
                ifb <p2>
                extrn `p1` : far
                else
                extrn "&p1",`p2` : far
                endif
                endm

lcall           macro   dest
                push    cs
                call    near ptr dest
                endm

else

modstart        macro   modname,alignment
ifdef NDEBUG
;                name    modname
endif
                if _MODEL and _USE_32_SEGS
_TEXT               segment use32 alignment public 'CODE'
                else
_TEXT               segment alignment public 'CODE'
                endif
                assume  cs:_TEXT
                endm

calli           macro   regname
                call    regname
                endm

codeptr         macro   p1,p2
                if _MODEL and _USE_32_SEGS
                    ifb <p2>
                    extrn p1 : dword
                    else
                    extrn "&p1",p2 : dword
                    endif
                else
                    ifb <p2>
                    extrn p1 : word
                    else
                    extrn "&p1",p2 : word
                    endif
                endif
                endm

defn            macro   dsym
         dsym   proc    near
                endm

defp            macro   dsym,exp
         dsym   proc    near exp
                endm

defpe           macro   dsym
                ifdef _EXPORT
         dsym       proc    near export
                else
         dsym       proc    near
                endif
                endm

docall          macro   dest
                call    dest
                endm

dojmp           macro   dest
                jmp     dest
                endm

endproc         macro   dsym
         dsym   endp
                endm

epilog          macro
                if _MODEL and _USE_32_SEGS
                    pop     EBP
                else
                    pop     BP
                endif
                endm

prolog          macro
                if _MODEL and _USE_32_SEGS
                    push    EBP
                    mov     EBP,ESP
                else
                    push    BP
                    mov     BP,SP
                endif
                endm

xdefp           macro   p1,p2
                ifb <p2>
                public p1
                else
                public "&p1",p2
                endif
                endm

xrefp           macro   p1,p2
                ifb <p2>
                extrn `p1` : near
                else
                extrn "&p1",`p2` : near
                endif
                endm

lcall           macro   dest
                call    dest
                endm

endif

endmod          macro
_TEXT           ends
                endm

dataseg         macro
DGROUP          group _DATA
assume          ds:DGROUP,ss:DGROUP
                if _MODEL and _USE_32_SEGS
_DATA               segment use32 dword public 'DATA'
                else
_DATA               segment word public 'DATA'
                endif
                endm
datasegment     macro
                dataseg ; should be phased out
                endm

enddata         macro
_DATA   ends
                endm

bss_segment     macro
                if _MODEL and _USE_32_SEGS
_BSS                segment use32 dword public 'BSS'
                else
_BSS                segment word public 'BSS'
                endif
DGROUP          group _BSS
                assume  ds:DGROUP
                endm

endbss          macro
_BSS            ends
                endm

alias_function  macro   alias, function
                ifb <function>
                    xrefp   "C",`_&alias`
                else
                    xrefp   "C",`function`
                endif
                .code
                public      "C",`alias`
                ifdef _EXPORT
         `alias`    proc    export
                else
         `alias`    proc
                endif
                ifb <function>
                    jmp     `_&alias`
                else
                    jmp     `function`
                endif
         `alias`    endp
                endm

xrefd           macro   p1,p2,p3
                ifb <p3>
                extrn p1 : p2
                else
                extrn "&p1",p2 : p3
                endif
                endm

