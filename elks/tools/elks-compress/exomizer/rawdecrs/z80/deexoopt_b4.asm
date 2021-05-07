;Exomizer 3 Z80 decoder
;Copyright (C) 2008-2018 by Jaime Tejedor Gomez (Metalbrain)
;
;Optimized by Antonio Villena and Urusergi
;
;Compression algorithm by Magnus Lind
;   exomizer raw -P15 -T1
;
;   This depacker is free software; you can redistribute it and/or
;   modify it under the terms of the GNU Lesser General Public
;   License as published by the Free Software Foundation; either
;   version 2.1 of the License, or (at your option) any later version.
;
;   This library is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;   Lesser General Public License for more details.
;
;   You should have received a copy of the GNU Lesser General Public
;   License along with this library; if not, write to the Free Software
;   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
;
; mapbase low byte must $00

        xor     a
        ld      a, 128
        ld      iyl, 240
        ld      b, 52
        push    de
init    ld      c, 16
        jr      nz, get4
        ld      de, 1
        ld      ixl, c
        defb    218
gb4     ld      a, (hl)
        dec     hl
get4    adc     a, a
        jr      z, gb4
        rl      c
        jr      nc, get4
        ld      iyh, mapbase/256
        ex      af, af'
        ld      a, c
        rrca
        inc     a
        ld      (iy+0), a
        jr      nc, get5
        xor     136
get5    push    hl
        ld      hl, 1
        defb    56
setbit  add     hl, hl
        dec     a
        jr      nz, setbit
        ex      af, af'
        inc     iyh
        ld      (iy+0), e
        inc     iyh
        ld      (iy+0), d
        add     hl, de
        ex      de, hl
        inc     iyl
        pop     hl
        dec     ixl
        djnz    init
        pop     de
litcop  ldd
mloop   add     a, a
        jr      z, gbm
        jr      c, litcop
gbmc    ld      bc, mapbase+240
        add     a, a
        jr      z, gbi2
        jr      c, gbic
getind  add     a, a
        jr      z, gbi
geti2   inc     c
        jr      nc, getind
        ret     z
gbic    and     a
        push    de
        ex      af, af'
        ld      a, (bc)
        ld      b, a
        ex      af, af'
        ld      de, 0
        dec     b
        call    nz, getbits
        ex      af, af'
        ld      b, (mapbase/256)+1
        ld      a, (bc)
        inc     b
        add     a, e
        ld      e, a
        ld      a, (bc)
        adc     a, d
        ld      d, a
        push    de
        ld      bc, 512+32
        dec     e
        jr      z, goit
        dec     e
        ld      bc, 1024+16
        jr      z, goit
        ld      c, 0
        ld      e, c
goit    ld      d, e
        ex      af, af'
        call    lee8
        ex      af, af'
        ld      a, e
        add     a, c
        ld      c, a
        ld      e, d
        ld      b, mapbase/256
        ld      a, (bc)
        ld      b, a
        ex      af, af'
        dec     b
        call    nz, getbits
        ex      af, af'
        ld      b, (mapbase/256)+1
        ld      a, (bc)
        inc     b
        add     a, e
        ld      e, a
        ld      a, (bc)
        adc     a, d
        ld      d, a
        ex      af, af'
        pop     bc
        ex      (sp), hl
        ex      de, hl
        add     hl, de
        lddr
        pop     hl
        jp      mloop

gbi     ld      a, (hl)
        dec     hl
        adc     a, a
        jr      nc, geti2
        inc     c
        jp      nz, gbic
        ret
gbi2    ld      a, (hl)
        dec     hl
        adc     a, a
        jr      nc, getind
        jp      gbic
gbm     ld      a, (hl)
        dec     hl
        adc     a, a
        jr      nc, gbmc
        jp      litcop

getbits jp      p, lee8
        rl      b
        jr      z, gby
        srl     b
        defb    $fa
xopy    ld      a, (hl)
        dec     hl
lee16   adc     a, a
        jr      z, xopy
        rl      d
        djnz    lee16
gby     ld      e, (hl)
        dec     hl
        ret

copy    ld      a, (hl)
        dec     hl
lee8    adc     a, a
        jr      z, copy
        rl      e
        djnz    lee8
        ret