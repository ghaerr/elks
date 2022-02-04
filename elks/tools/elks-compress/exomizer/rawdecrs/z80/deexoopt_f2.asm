;Exomizer 3 Z80 decoder
;Copyright (C) 2008-2018 by Jaime Tejedor Gomez (Metalbrain)
;
;Optimized by Antonio Villena and Urusergi
;
;Compression algorithm by Magnus Lind
;   exomizer raw -P13 -T0 (literals=1) (reuse=0)
;   exomizer raw -P13 -T1 (literals=0) (reuse=0)
;   exomizer raw -P45 -T0 (literals=1) (reuse=1)
;   exomizer raw -P45 -T1 (literals=0) (reuse=1)
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
        inc     c
      IF  mapbase-mapbase/256*256<240 AND mapbase-mapbase/256*256>135
        ld      (iy-256+mapbase-mapbase/256*256), c
      ELSE
        ld      (iy-112+mapbase-(mapbase+16)/256*256), c
      ENDIF
        push    hl
        ld      hl, 1
        defb    48
setbit  add     hl, hl
        dec     c
        jr      nz, setbit
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
        jr      z, gbm
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
        call    getpair
    IF  reuse=1
      IF  mapbase-mapbase/256*256<240 AND mapbase-mapbase/256*256>135
        rl      (ix-152+mapbase-mapbase/256*256)
      ELSE
        rl      (ix-6+mapbase-(mapbase+16)/256*256)
      ENDIF
        and     a
    ENDIF
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
        call    getbits
        ld      iyl, c
        add     iy, de
        ld      e, d
        call    getpair
      IF  reuse=1
        ld      (mapbase+156), de
caof    and     a
      ELSE
caof
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
        push    de
        ld      de, 0
        ld      b, 16
        call    getbits
        ld      b, d
        ld      c, e
        pop     de
      IF  reuse=1
        scf
      ENDIF
        ldir
        jr      mloo1
    ENDIF

      IF  reuse=1
gba     ld      a, (hl)
        inc     hl
        adc     a, a
        jr      nc, aqui
        jp      caof
      ENDIF

gbm     ld      a, (hl)
        inc     hl
        adc     a, a
        jr      nc, gbmc
        jp      litcop

gbi     ld      a, (hl)
        inc     hl
        adc     a, a
        jp      gbic

      IF  mapbase-mapbase/256*256<240 AND mapbase-mapbase/256*256>135
getpair ld      b, (iy-256+mapbase-mapbase/256*256)
      ELSE
getpair ld      b, (iy-112+mapbase-(mapbase+16)/256*256)
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
        ex      de, hl
        ret

gbg     ld      a, (hl)
        inc     hl
getbits adc     a, a
        jr      z, gbg
        rl      e
        rl      d
        djnz    getbits
        ret