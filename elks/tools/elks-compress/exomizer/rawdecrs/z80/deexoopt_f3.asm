;Exomizer 3 Z80 decoder
;Copyright (C) 2008-2018 by Jaime Tejedor Gomez (Metalbrain)
;
;Optimized by Antonio Villena and Urusergi
;
;Compression algorithm by Magnus Lind
;   exomizer raw -P15 -T0 (literals=1) (reuse=0)
;   exomizer raw -P15 -T1 (literals=0) (reuse=0)
;   exomizer raw -P47 -T0 (literals=1) (reuse=1)
;   exomizer raw -P47 -T1 (literals=0) (reuse=1)
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
      IF  mapbase-mapbase/256*256<240 AND mapbase-mapbase/256*256>135
        ld      iy, 256+mapbase/256*256
      ELSE
        ld      iy, (mapbase+16)/256*256+112
      ENDIF
        ld      a, 128
        ld      b, 52
        push    de
        cp      a
init    ld      c, 16
        jr      nz, get4
        ld      de, 1
        ld      ixl, c
        defb    218
gb4     ld      a, (hl)
        inc     hl
get4    adc     a, a
        jr      z, gb4
        rl      c
        jr      nc, get4
        ex      af, af'
        ld      a, c
        rrca
        inc     a
      IF  mapbase-mapbase/256*256<240 AND mapbase-mapbase/256*256>135
        ld      (iy-256+mapbase-mapbase/256*256), a
      ELSE
        ld      (iy-112+mapbase-(mapbase+16)/256*256), a
      ENDIF
        jr      nc, get5
        xor     136
get5    push    hl
        ld      hl, 1
        defb    56
setbit  add     hl, hl
        dec     a
        jr      nz, setbit
        ex      af, af'
      IF  mapbase-mapbase/256*256<240 AND mapbase-mapbase/256*256>135
        ld      (iy-204+mapbase-mapbase/256*256), e
        ld      (iy-152+mapbase-mapbase/256*256), d
      ELSE
        ld      (iy-60+mapbase-(mapbase+16)/256*256), e
        ld      (iy-8+mapbase-(mapbase+16)/256*256), d
      ENDIF
        add     hl, de
        ex      de, hl
        inc     iyl
        pop     hl
        dec     ixl
        djnz    init
    IF  reuse=1
        push    iy
        pop     ix
      IF  mapbase-mapbase/256*256<240 AND mapbase-mapbase/256*256>135
        ld      (ix-152+mapbase-mapbase/256*256), 1
      ELSE
        ld      (ix-6+mapbase-(mapbase+16)/256*256), 1
      ENDIF
        scf
    ENDIF
        pop     de
litcop  ldi
mloo1
    IF  reuse=1
      IF  mapbase-mapbase/256*256<240 AND mapbase-mapbase/256*256>135
        rl      (ix-152+mapbase-mapbase/256*256)
      ELSE
        rl      (ix-6+mapbase-(mapbase+16)/256*256)
      ENDIF
    ENDIF
mloop   add     a, a
      IF  reuse=0
        jr      z, gbm
      ELSE
        jr      nz, gbm
        ld      a, (hl)
        inc     hl
        adc     a, a
gbm 
      ENDIF
        jr      c, litcop
gbmc    
      IF  mapbase-mapbase/256*256<240 AND mapbase-mapbase/256*256>135
        ld      c, 256-1
      ELSE
        ld      c, 112-1
      ENDIF
getind  add     a, a
        jr      z, gbi
gbic    inc     c
        jr      nc, getind
        ccf
    IF  mapbase-mapbase/256*256<240 AND mapbase-mapbase/256*256>135
        bit     4, c
      IF  literals=1
        jr      nz, litcat
      ELSE
        ret     nz
      ENDIF
    ELSE
      IF  literals=1
        jp      m, litcat
      ELSE
        ret     m
      ENDIF
    ENDIF
        push    de
        ld      iyl, c
        ld      de, 0
      IF  mapbase-mapbase/256*256<240 AND mapbase-mapbase/256*256>135
        ld      b, (iy-256+mapbase-mapbase/256*256)
      ELSE
        ld      b, (iy-112+mapbase-(mapbase+16)/256*256)
      ENDIF
        dec     b
        call    nz, getbits
        ex      de, hl
      IF  mapbase-mapbase/256*256<240 AND mapbase-mapbase/256*256>135
        ld      c, (iy-204+mapbase-mapbase/256*256)
        ld      b, (iy-152+mapbase-mapbase/256*256)
      ELSE
        ld      c, (iy-60+mapbase-(mapbase+16)/256*256)
        ld      b, (iy-8+mapbase-(mapbase+16)/256*256)
      ENDIF
        add     hl, bc
    IF  reuse=1
      IF  mapbase-mapbase/256*256<240 AND mapbase-mapbase/256*256>135
        rl      (ix-152+mapbase-mapbase/256*256)
      ELSE
        rl      (ix-6+mapbase-(mapbase+16)/256*256)
      ENDIF
        and     a
    ENDIF
        ex      de, hl
        push    de
      IF  mapbase-mapbase/256*256<240 AND mapbase-mapbase/256*256>135
        ld      bc, 512+48
        dec     e
        jr      z, goit
        dec     e
        ld      bc, 1024+32
        jr      z, goit
        ld      c, 16
      ELSE
        ld      bc, 512+160
        dec     e
        jr      z, goit
        dec     e
        ld      bc, 1024+144
        jr      z, goit
        ld      c, 128
      ENDIF
    IF  reuse=1
goit
      IF  mapbase-mapbase/256*256<240 AND mapbase-mapbase/256*256>135
        ld      e, (ix-152+mapbase-mapbase/256*256)
      ELSE
        ld      e, (ix-6+mapbase-(mapbase+16)/256*256)
      ENDIF
        bit     1, e
        jr      z, aqui
        bit     2, e
        jr      nz, aqui
        add     a, a
        ld      de, (mapbase+156)
        jr      z, gba
        jr      c, caof
aqui    ld      de, 0
    ELSE
        ld      e, 0
goit    ld      d, e
    ENDIF
        call    lee8
        ld      iyl, c
        add     iy, de
        ld      e, d
      IF  mapbase-mapbase/256*256<240 AND mapbase-mapbase/256*256>135
        ld      b, (iy-256+mapbase-mapbase/256*256)
      ELSE
        ld      b, (iy-112+mapbase-(mapbase+16)/256*256)
      ENDIF
        dec     b
        call    nz, getbits
        ex      de, hl
      IF  mapbase-mapbase/256*256<240 AND mapbase-mapbase/256*256>135
        ld      c, (iy-204+mapbase-mapbase/256*256)
        ld      b, (iy-152+mapbase-mapbase/256*256)
      ELSE
        ld      c, (iy-60+mapbase-(mapbase+16)/256*256)
        ld      b, (iy-8+mapbase-(mapbase+16)/256*256)
      ENDIF
        add     hl, bc
      IF  reuse=0
        ex      de, hl
caof    
      ELSE
        ld      (mapbase+156), hl
        ex      de, hl
caof    and     a
      ENDIF
        pop     bc
        ex      (sp), hl
        push    hl
        sbc     hl, de
        pop     de
        ldir
        pop     hl
        jr      mloop

    IF  literals=1
litcat  
      IF  mapbase-mapbase/256*256<240 AND mapbase-mapbase/256*256>135
        rl      c
      ENDIF
        ret     pe
        ld      b, (hl)
        inc     hl
        ld      c, (hl)
        inc     hl
        ldir
      IF  reuse=0
        jr      mloo1
      ELSE
        scf
        jp      mloo1
      ENDIF
    ENDIF

gbi     ld      a, (hl)
        inc     hl
        adc     a, a
        jp      gbic

      IF  reuse=0
gbm     ld      a, (hl)
        inc     hl
        adc     a, a
        jr      nc, gbmc
        jp      litcop
      ELSE
gba     ld      a, (hl)
        inc     hl
        adc     a, a
        jr      nc, aqui
        jp      caof
      ENDIF

getbits jp      p, lee8
        rl      b
        jr      z, gby
        srl     b
        defb    250
xopy    ld      a, (hl)
        inc     hl
lee16   adc     a, a
        jr      z, xopy
        rl      d
        djnz    lee16
gby     ld      e, (hl)
        inc     hl
        ret

copy    ld      a, (hl)
        inc     hl
lee8    adc     a, a
        jr      z, copy
        rl      e
        djnz    lee8
        ret