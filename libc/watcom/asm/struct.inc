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
;* Description:  Structured macros for MASM/WASM.
;*
;*****************************************************************************


        __label = 0
        __depth = 0
        __inner_loop = 0

_guess  macro   name
        __depth = __depth + 1
        __label = __label + 1
        _set    __astk,%__depth,__label,<>
        __label = __label + 1
        _set    __estk,%__depth,__label,<>
        ifnb    <name>
          name = __label
        endif
        endm

_quif   macro   cc,name
        ifnb    <name>
          _set  __elbl,,name,<>
          _j    cc,_l,%__elbl
        else
          _set  __albl,,__astk,%__depth
          _j    cc,_l,%__albl
        endif
        endm

_quit   macro   name
        _quif   ,name
        endm

_admit  macro
        _set    __albl,,__astk,%__depth
        __label = __label + 1
        _set    __astk,%__depth,__label,<>
        _set    __elbl,,__estk,%__depth
        _j      ,_l,%__elbl
        _label  _l,%__albl
        endm

_endguess macro
        _set    __albl,,__astk,%__depth
        _label  _l,%__albl
        _set    __elbl,,__estk,%__depth
        _label  _l,%__elbl
        __depth = __depth - 1
        endm

_loop   macro   name
        _guess  name
        _set    __albl,,__astk,%__depth
        _label  _m,%__albl
        _set    __elbl,,__estk,%__depth
        _label  _m,%__elbl
        _set    __llbl,%__depth,%__inner_loop,<>
        _set    __inner_loop,,%__depth,<>
        endm

_loopif macro   cc,name
        ifnb    <name>
          _set  __elbl,,name,<>
          _j    cc,_m,%__elbl
        else
          _set  __albl,,__astk,%__inner_loop
          _j    cc,_m,%__albl
        endif
        endm

_until  macro   cc
        _set    __albl,,__astk,%__depth
        _jn     cc,_m,%__albl
        _set    __inner_loop,,__llbl,%__depth
        _endguess
        endm

_endloop macro
        _set    __albl,,__astk,%__depth
        _j      ,_m,%__albl
        _set    __inner_loop,,__llbl,%__depth
        _endguess
        endm

_if     macro   cc
        _guess
        _set    __albl,,__astk,%__depth
        _jn     cc,_l,%__albl
        endm

_else   macro
        _admit
        endm

_endif  macro
        _endguess
        endm

_set    macro   base1,ext1,base2,ext2
        base1&ext1 = base2&ext2
        endm

_label  macro   base,ext
base&ext:
        endm

_j      macro   cc,base,ext
        j&cc        base&ext
        endm

_jn     macro   cc,base,ext
        jn&cc       base&ext
        endm

jnna    macro   label
        ja      label
        endm

jnnae   macro   label
        jae     label
        endm

jnnb    macro   label
        jb      label
        endm

jnnbe   macro   label
        jbe     label
        endm

jnnc    macro   label
        jc      label
        endm

jnne    macro   label
        je      label
        endm

jnng    macro   label
        jg      label
        endm

jnnge   macro   label
        jge     label
        endm

jnnl    macro   label
        jl      label
        endm

jnnle   macro   label
        jle     label
        endm

jnno    macro   label
        jo      label
        endm

jnnp    macro   label
        jp      label
        endm

jnns    macro   label
        js      label
        endm

jnnz    macro   label
        jz      label
        endm

jnpe    macro   label
        jpo     label
        endm

jnpo    macro   label
        jpe     label
        endm

j       macro   label
        jmp     label
        endm

jn      macro   label
        nop
        endm

_shl    macro   reg,count
        add     reg,reg
        endm

_rcl    macro   reg,count
        adc     reg,reg
        endm
